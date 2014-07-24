#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <linux/fb.h>

#include "lib.h"
#include "command.h"
#include "logmessage.h"

#define LCD_LIST_FILE		"/usr/local/lcd-list.dat"
#define LCD_LIST_FILE_TMP	"/tmp/lcd-list.dat"
#define TMP_PACKIMG_DEV_NAME	"/dev/mtd-packimg"

#define LCD_INFO_NAME_SIZE	128

#define LOCAL_LCD_MODE_TTL		0
#define LOCAL_LCD_MODE_LVDS6	1
#define LOCAL_LCD_MODE_LVDS8	2

struct local_lcd_info{
	char name[LCD_INFO_NAME_SIZE];
	int mode;
	struct fb_info info;
};

struct lcd_info_string{
	char *name;
	char *param;
};

static int match_em6000(const char *platform)
{
	return strncmp(platform, "EM6", 3)==0;
}

#define CASE_CHAR2VAL(C, c, v, p)	\
		case C: (p)|=(v); (p)|=POL_VALID; break; \
		case c: (p)&=~(v); (p)|=POL_VALID; break;

static char *get_lcd_pol(char* str, enum fb_pol *pol, int *mode)
{
	*pol = POL_DEFALT;

	//for BODPHV
	while(*str!=0){
		if(*str=='\n'){
			str++;
			break;
		}
		switch(*str){
			CASE_CHAR2VAL('B', 'b', POL_BL, *pol)
			CASE_CHAR2VAL('O', 'o', POL_LCDONOFF, *pol)
			CASE_CHAR2VAL('D', 'd', POL_DE, *pol)
			CASE_CHAR2VAL('P', 'p', POL_PCLK, *pol)
			CASE_CHAR2VAL('H', 'h', POL_HS, *pol)
			CASE_CHAR2VAL('V', 'v', POL_VS, *pol)
			case 'L':
				*mode = LOCAL_LCD_MODE_LVDS8;
				break;
			case 'l':
				*mode = LOCAL_LCD_MODE_LVDS6;
				break;
		}
		str++;
	}
	return str;
}

static int str2fbinfo(const char *buf, struct local_lcd_info *lcdinfo)
{
	int ret;
	char suffix[128];
	struct fb_info *fi=&lcdinfo->info;

	lcdinfo->mode=LOCAL_LCD_MODE_TTL;
	
	/*
		[width]x[height]@[PCLK(KHz)],[HFP]:[HSW]:[HBP],[VFP]:[VSW]:[VBP],BODPHV(L)
																		L: 8bit lvds, l: 6bit lvds, none: TTL
	*/
	
	ret = sscanf(buf, "%dx%d@%d,%d:%d:%d,%d:%d:%d%[^#]",
		&fi->width, &fi->height, &fi->pclk, 
		&fi->hfp, &fi->hsw, &fi->hbp,
		&fi->vfp, &fi->vsw, &fi->vbp, suffix);
	if(ret<9)
		return -1;

	if(ret==9)
		fi->pol=0;
	else
		get_lcd_pol(suffix, &fi->pol, &lcdinfo->mode);

	return 0;
}

//return match lcdindex or -1
static int em6000_load_lcds(int *nlcd, struct local_lcd_info *lcdinfo, struct fb_info *fbinfo)
{
	FILE *pf;
	char buf[MAX_STRING], *p;
	int ret, i, v, match=-1, n;

	if(fbinfo){
		if(get_fb_info(fbinfo)){
			FAILED_OUT("get framebuffer info");
			return -1;
		}
		//need it, hbp = hbp+hsw
	//	fbinfo->hbp -= fbinfo->hsw;
	//	fbinfo->vbp -= fbinfo->vsw;
	}

	pf = fopen(LCD_LIST_FILE, "r");
	if(pf==NULL){
		FAILED_OUT("can't find lcd data file");
		return -1;
	}

	i=0;
	while(fgets(buf, sizeof(buf), pf)){
		for(p=buf; *p==' ' || *p=='\t'; p++);
		if(*p=='#')	//ignore line
			continue;

		ret = sscanf(p, "%s%n", lcdinfo->name, &n);
		if(ret!=1)
			continue;
		p += n;

		if(str2fbinfo(p, lcdinfo)<0 || !fbinfo)
			continue;

		v = lcdinfo->info.pclk >= fbinfo->pclk ? (lcdinfo->info.pclk - fbinfo->pclk) : 
			fbinfo->pclk - lcdinfo->info.pclk;

		if(fbinfo->width == lcdinfo->info.width && 
				fbinfo->height == lcdinfo->info.height &&
				fbinfo->hfp == lcdinfo->info.hfp &&
				fbinfo->hsw == lcdinfo->info.hsw &&
				fbinfo->hbp == lcdinfo->info.hbp &&
				fbinfo->vfp == lcdinfo->info.vfp &&
				fbinfo->vsw == lcdinfo->info.vsw &&
				fbinfo->vbp == lcdinfo->info.vbp &&
				v < fbinfo->pclk*5/100){

			match = i;
		}

		lcdinfo++;
		i++;
		if(i==*nlcd)
			break;
	}
	close(pf);

	*nlcd = i;
	return match;
}

static void output_lcdinfo(struct local_lcd_info *plcdinfo, FILE *f)
{
	enum fb_pol pol;
	char suffix[128];

	pol = plcdinfo->info.pol;
	if(pol&POL_VALID){
		snprintf(suffix, sizeof(suffix), ",%c%c%c%c%c%c%s", 
			(pol&POL_BL)?'B':'b',
			(pol&POL_LCDONOFF)?'O':'o',
			(pol&POL_DE)?'D':'d',
			(pol&POL_PCLK)?'P':'p',
			(pol&POL_HS)?'H':'h',
			(pol&POL_VS)?'V':'v', 
			(plcdinfo->mode==LOCAL_LCD_MODE_TTL)?"":(plcdinfo->mode==LOCAL_LCD_MODE_LVDS8)?"L":"l");
	}
	else{
		suffix[0]=0;
	}

	fprintf(f, "%s %dx%d@%d,%d:%d:%d,%d:%d:%d%s\n", plcdinfo->name, 
		plcdinfo->info.width, plcdinfo->info.height, plcdinfo->info.pclk, 
		plcdinfo->info.hfp, plcdinfo->info.hsw, plcdinfo->info.hbp,
		plcdinfo->info.vfp, plcdinfo->info.vsw, plcdinfo->info.vbp, suffix);
}

#define MAX_LCD_LIST	128

static int em6000_save_lcds(struct lcd_info_string *lcdinfo_str, int n, 
		struct local_lcd_info *pflcdinfo, int fn)
{
	char *lcdparam, *lcdname;
	int i, j;

	//copy into tmp
	if(run_cmd_quiet(NULL, NULL, "cp "LCD_LIST_FILE" "LCD_LIST_FILE_TMP)<0)
		return -1;

	//remove deleted lcd
	for(j=0;j<fn;j++){
		lcdname = pflcdinfo[j].name;
		for(i=0;i<n;i++){
			if(strcmp(lcdinfo_str[i].name, lcdname)==0){
				break;
			}
		}
		if(i==n){//not find in new lcd list, remove it
			if(run_cmd_quiet(NULL, NULL, 
				"sed -i -e 's/^[[:space:]]*//g;/^\\<%s\\>.*$/d' "LCD_LIST_FILE_TMP, lcdname)<0)
				return -1;
		}
	}

	//add or replace to new
	for(i=0;i<n;i++){
		lcdname = lcdinfo_str[i].name;
		lcdparam = lcdinfo_str[i].param;

		if(run_cmd_quiet(NULL, NULL, 
			"sed -e 's/^[[:space:]]*//g' "LCD_LIST_FILE_TMP" | grep '^\\<%s\\>'>/dev/null && "
			"sed -i -e 's/^[[:space:]]*//g;s/^\\<%s\\>.*$/%s %s/g' "LCD_LIST_FILE_TMP" || "
			"echo \"%s %s\" >> "LCD_LIST_FILE_TMP, 
			lcdname, lcdname, lcdname, lcdparam, lcdname, lcdparam)<0)
			return -1;
	}

	if(run_cmd_quiet(NULL, NULL, "mv "LCD_LIST_FILE_TMP" "LCD_LIST_FILE)<0)
		return -1;

	return 0;
}

static void em6000_get_lcd(int argc, char *argv[])
{
	struct local_lcd_info *plcdinfo=malloc(MAX_LCD_LIST*sizeof(struct local_lcd_info));
	struct fb_info fbinfo;
	int match, n;
	
	LOG("%s\n", __FUNCTION__);
	if(!plcdinfo){
		FAILED_OUT("alloc lcdinfo");
		return;
	}

	n = MAX_LCD_LIST;
	match = em6000_load_lcds(&n, plcdinfo, &fbinfo);

	if(match<0)
		printf("unknown %dx%d@%d,%d:%d:%d,%d:%d:%d\n", 
					fbinfo.width, fbinfo.height, fbinfo.pclk, 
					fbinfo.hfp, fbinfo.hsw, fbinfo.hbp,
					fbinfo.vfp, fbinfo.vsw, fbinfo.vbp);
	else{
		output_lcdinfo(&plcdinfo[match], stdout);
	}

	free(plcdinfo);
}

BUILDIN_CMD("get-lcd", em6000_get_lcd, match_em6000);

static void em6000_list_lcd(int argc, char *argv[])
{
	struct local_lcd_info *plcdinfo=malloc(MAX_LCD_LIST*sizeof(struct local_lcd_info));
	struct fb_info fbinfo;
	int match, n, i;
	
	LOG("%s\n", __FUNCTION__);
	if(!plcdinfo){
		FAILED_OUT("alloc lcdinfo");
		return;
	}

	n = MAX_LCD_LIST;
	match = em6000_load_lcds(&n, plcdinfo, &fbinfo);

	if(match<0)
		printf("*unknown %dx%d@%d,%d:%d:%d,%d:%d:%d\n", 
					fbinfo.width, fbinfo.height, fbinfo.pclk, 
					fbinfo.hfp, fbinfo.hsw, fbinfo.hbp,
					fbinfo.vfp, fbinfo.vsw, fbinfo.vbp);

	for(i=0;i<n;i++){
		putchar(i==match?'*':' ');
		output_lcdinfo(&plcdinfo[i], stdout);
	}

	free(plcdinfo);
}
BUILDIN_CMD("list-lcd", em6000_list_lcd, match_em6000);

#define MAX_PACK		5
#define MAX_PACK_NAME	32

struct unpackimg_out_t{
	char filename[MAX_PACK_NAME];
	unsigned int loadaddr;
	char packimg_name_addr[MAX_STRING];
};

struct packimg_name_map{
	const unsigned int loadaddr;
	const char name[MAX_PACK_NAME];
};

static const struct packimg_name_map packname_map[]={
	{0x43100000, "splash"},
	{0x43000000, "script"},
	{0x44000000, "fdt"},
};

static unsigned int find_packaddr_by_name(const char *name)
{
	int i;
	for(i=0;i<ARRAY_SIZE(packname_map);i++){
		if(strncmp(name, packname_map[i].name,MAX_PACK_NAME)==0)
			return packname_map[i].loadaddr;
	}

	return 0;
}

static int unpackimg_out_cbk(struct unpackimg_out_t *pack, char out[], int n)
{
	unsigned int addr=0;
	char filename[MAX_PACK_NAME];
	int ret;

	ret = sscanf(out, "%[^@]@%x", filename, &addr);
	if(ret<2)
		return 0;

	ret = strlen(pack->packimg_name_addr);
	snprintf(pack->packimg_name_addr+ret, MAX_STRING-ret, 
		" %s@0x%x", filename, addr);
	
	if(pack->loadaddr!=addr)
		return 0;

	//find load addr
	strncpy(pack->filename, filename, sizeof(filename));

	return 0;
}

#define create_packimg_node(n)	run_cmd_quiet(NULL, NULL, "mknod "TMP_PACKIMG_DEV_NAME" c 90 %d", n*2)
#define remove_packimg_node()	run_cmd_quiet(NULL, NULL, "rm "TMP_PACKIMG_DEV_NAME)

#define AWK_INI_FILE_BEGIN(sec)	"awk -F \'=\' \'/^\\[/{a=0}/\\["sec"\\]/{a=1}a==1{"
#define AWK_INI_FILE_KEY(key)	"if($1==\""key"\")print $1\"=%d\";else "
#define AWK_INI_FILE_END()	"print $0;next;}{print $0}\'"

static int set_lcd_info(struct fb_info *fbinfo, int mode)
{
	struct unpackimg_out_t pack;
	const char *devnum = GET_CONF_VALUE(PACKIMG_DEVNUM);
	unsigned int ht, vt;

	memset(&pack, 0, sizeof(pack));
	pack.loadaddr = find_packaddr_by_name("script");

	chdir(GET_CONF_VALUE(TMPFILE_PATH));
	//unpack head
	if (run_cmd_quiet((out_callback_fun)unpackimg_out_cbk, &pack, 
		"unpackimg -d /dev/blockrom%s", devnum) < 0) {
		return -1;
	}

	if(pack.filename[0]==0){
		FAILED_OUT("can't find script");
		return -1;
	}

	ht = fbinfo->hfp + fbinfo->hbp + fbinfo->width + fbinfo->hsw;
	vt = fbinfo->vfp + fbinfo->vbp + fbinfo->height + fbinfo->vsw;;
//	printf("%d,%d,%d\n", fbinfo->vfp ,fbinfo->vbp ,fbinfo->height);
	vt <<=1;

	if(run_cmd_quiet(NULL, NULL, "bin2fex %s 2>/dev/null |sed -e \'s/[[:space:]]*//g\'|"
		AWK_INI_FILE_BEGIN("lcd0_para")
		AWK_INI_FILE_KEY("lcd_x")
		AWK_INI_FILE_KEY("lcd_y")
		AWK_INI_FILE_KEY("lcd_dclk_freq")
		AWK_INI_FILE_KEY("lcd_hbp")
		AWK_INI_FILE_KEY("lcd_ht")
		AWK_INI_FILE_KEY("lcd_vbp")
		AWK_INI_FILE_KEY("lcd_vt")
		AWK_INI_FILE_KEY("lcd_hv_hspw")
		AWK_INI_FILE_KEY("lcd_hv_vspw")
		AWK_INI_FILE_KEY("lcd_if")
		AWK_INI_FILE_KEY("lcd_lvds_bitwidth")
		AWK_INI_FILE_KEY("lcd_frm")
		AWK_INI_FILE_KEY("lcd_io_cfg0")
		AWK_INI_FILE_END()
		"|fex2bin > %stmp",
		pack.filename, 
		fbinfo->width, fbinfo->height, (fbinfo->pclk+500)/1000, 
		fbinfo->hbp+fbinfo->hsw, ht, 
		fbinfo->vbp+fbinfo->vsw, vt, fbinfo->hsw, fbinfo->vsw,
		(mode==LOCAL_LCD_MODE_TTL)?0:3,(mode==LOCAL_LCD_MODE_LVDS8)?0:1,(mode==LOCAL_LCD_MODE_TTL)?0:1,
		(fbinfo->pol & POL_PCLK)?0:0x10000000,
		pack.filename)<0){
		return -1;
	}

	if(create_packimg_node(atoi(devnum))<0){
		FAILED_OUT("create packimg node");
		return -1;
	}
		
	if(run_cmd_quiet(NULL, NULL, "mv %stmp %s &&"
		"packimg -d "TMP_PACKIMG_DEV_NAME" %s pack.img && "
		"packimg_burn -d "TMP_PACKIMG_DEV_NAME" -c5 -f pack.img",
		pack.filename,pack.filename,pack.packimg_name_addr)<0){
		remove_packimg_node();
		return -1;
	}
	
	remove_packimg_node();
	return 0;	
}

static void em6000_set_lcd(int argc, char *argv[])
{
	enum lcdinfo_mode{
		LCDMODE_SET,
		LCDMODE_SETINDEX,
		LCDMODE_SAVE,
	}mode = LCDMODE_SET;

	static char lcd_data_string[MAX_LCD_LIST*256], *p=lcd_data_string;
	static int count=0;
	static struct lcd_info_string lcdinfo_str[MAX_LCD_LIST];

	struct local_lcd_info *plcdinfo;

	const char*const short_options ="i:rs";
	struct fb_info fbinfo;
	int nlcd, i, match, next_option;
	
	LOG("%s\n", __FUNCTION__);

	optind=0;
	while((next_option =getopt(argc,argv,short_options))!=-1) {
		switch (next_option){
		case 'i':
			mode = LCDMODE_SETINDEX;
			i = strtol(optarg, NULL, 0);
			break; 
		case 'r':	//remove all
			count = 0;
			p = lcd_data_string;
			return;
		case 's':
			mode = LCDMODE_SAVE;
			break;
		case -1:	//Done with options.
			break; 
		default:
			FAILED_OUT("arguments error");
			return;
		}
	}

	if(mode==LCDMODE_SET){
		if(argc < optind+2){
			FAILED_OUT("too few arguments");
			return;
		}

		strcpy(p, argv[optind]);
		lcdinfo_str[count].name = p;
		p+=strlen(argv[optind])+1;

		strcpy(p, argv[optind+1]);
		lcdinfo_str[count].param = p;
		p+=strlen(argv[optind+1])+1;

		count++;
		return;
	}

	plcdinfo=malloc(MAX_LCD_LIST*sizeof(struct local_lcd_info));
	if(!plcdinfo)
		return;

	nlcd=MAX_LCD_LIST;
	match = em6000_load_lcds(&nlcd, plcdinfo, &fbinfo);

	switch(mode){
	case LCDMODE_SETINDEX:
		if(i>=nlcd){
			FAILED_OUT("out of lcd list range");
			goto out;
		}
		if(set_lcd_info(&plcdinfo[i].info, plcdinfo[i].mode)<0)
			goto out;
		SUCESS_OUT();
		goto out;
	case LCDMODE_SAVE:
		if(em6000_save_lcds(lcdinfo_str,count, plcdinfo, nlcd)==0)
			SUCESS_OUT();
		goto out;
	}

out:
	free(plcdinfo);
}
BUILDIN_CMD("set-lcd", em6000_set_lcd, match_em6000);

static void get_splash(int argc, char *argv[])
{
	struct fb_info fbinfo;
	struct unpackimg_out_t pack;
	const char *devnum = GET_CONF_VALUE(PACKIMG_DEVNUM);

	LOG("%s\n", __FUNCTION__);
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	memset(&pack, 0, sizeof(pack));
	pack.loadaddr = find_packaddr_by_name("splash");

	chdir(GET_CONF_VALUE(TMPFILE_PATH));
	//unpack head
	if (run_cmd_quiet((out_callback_fun)unpackimg_out_cbk, &pack, 
		"unpackimg -d /dev/blockrom%s", devnum) < 0) {
		return;
	}

	if(pack.filename[0]==0){
		FAILED_OUT("can't find splash");
		return;
	}

	//we get splash.bin file at chdir now
	if(get_fb_info(&fbinfo)<0)
		return;

	if(run_cmd_quiet(NULL, NULL, "mksplash -t tmpfile -rd -b%d -o %s %s", 
		fbinfo.bpp, argv[1],pack.filename)<0)
		return;

	SUCESS_OUT();
}
BUILDIN_CMD("get-splash", get_splash, match_em6000);

static void set_splash(int argc, char *argv[])
{
	struct unpackimg_out_t pack;
	const char *devnum = GET_CONF_VALUE(PACKIMG_DEVNUM);
	struct fb_info fbinfo;

	int ret;
	unsigned long size;

	LOG("%s\n", __FUNCTION__);
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	if(get_fb_info(&fbinfo)<0)
		return;

	memset(&pack, 0, sizeof(pack));
	pack.loadaddr = find_packaddr_by_name("splash");

	chdir(GET_CONF_VALUE(TMPFILE_PATH));
	//unpack head
	if (run_cmd_quiet((out_callback_fun)unpackimg_out_cbk, &pack, 
		"unpackimg -d /dev/blockrom%s", devnum) < 0) {
		return;
	}

	if(pack.filename[0]==0){
		LOG("can't find splash");
		strcpy(pack.filename, "splash.bin");
	}

	if(run_cmd_quiet(NULL, NULL, "mksplash -t tmpfile -d -b%d -o %s %s", 
		fbinfo.bpp, pack.filename, argv[1])<0)
		return;

	if(create_packimg_node(atoi(devnum))<0){
		FAILED_OUT("create packimg node");
		return;
	}

	if(run_cmd_quiet(NULL, NULL, "packimg -d "TMP_PACKIMG_DEV_NAME" %s pack.img && "
		"packimg_burn -d "TMP_PACKIMG_DEV_NAME" -c5 -f pack.img",
		pack.packimg_name_addr)<0){
		remove_packimg_node();
		return;
	}
	
	remove_packimg_node();
	SUCESS_OUT();
}
BUILDIN_CMD("set-splash", set_splash, match_em6000);
