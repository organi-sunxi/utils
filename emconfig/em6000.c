#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <linux/fb.h>

#include "lib.h"
#include "command.h"
#include "logmessage.h"

#define LCD_LIST_FILE	"/usr/local/lcd-list.dat"

struct local_lcd_info{
	char name[128];
	struct fb_info info;
};

static int match_em6000(const char *platform)
{
	return strncmp(platform, "EM6", 3)==0;
}

#define CASE_CHAR2VAL(C, c, v, p)	\
		case C: (p)|=(v); (p)|=POL_VALID; break; \
		case c: (p)&=~(v); (p)|=POL_VALID; break;

static char *get_lcd_pol(char* str, enum fb_pol *pol)
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
			CASE_CHAR2VAL('H', 'h', POL_VS, *pol)
			CASE_CHAR2VAL('V', 'v', POL_VS, *pol)
		}
		str++;
	}
	return str;
}

static int str2fbinfo(const char *buf, struct fb_info *fi)
{
	int ret;
	char suffix[128];
	
	//[width]x[height]@[PCLK(KHz)],[HFP]:[HSW]:[HBP],[VFP]:[VSW]:[VBP],BODPHV
	ret = sscanf(buf, "%dx%d@%d,%d:%d:%d,%d:%d:%d%[^#]",
		&fi->width, &fi->height, &fi->pclk, 
		&fi->hfp, &fi->hsw, &fi->hbp,
		&fi->vfp, &fi->vsw, &fi->vbp, suffix);
	if(ret<9)
		return -1;

	if(ret==9)
		fi->pol=0;
	else
		get_lcd_pol(suffix, &fi->pol);

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
			return;
		}
	}

	pf = fopen(LCD_LIST_FILE, "r");
	if(pf==NULL){
		FAILED_OUT("can't find lcd data file");
		return;
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

		if(str2fbinfo(p, &lcdinfo->info)<0 || !fbinfo)
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
		snprintf(suffix, sizeof(suffix), ",%c%c%c%c%c%c", 
			(pol&POL_BL)?'B':'b',
			(pol&POL_LCDONOFF)?'O':'o',
			(pol&POL_DE)?'D':'d',
			(pol&POL_PCLK)?'P':'p',
			(pol&POL_HS)?'H':'h',
			(pol&POL_VS)?'V':'v');
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

static int set_lcd_info(struct fb_info *fbinfo)
{

}

static void em6000_set_lcd(int argc, char *argv[])
{
	enum lcdinfo_mode{
		LCDMODE_SET,
		LCDMODE_SETCUR,
		LCDMODE_SETINDEX,
		LCDMODE_RM,
	}mode = LCDMODE_SET;

	struct local_lcd_info *plcdinfo=malloc(MAX_LCD_LIST*sizeof(struct local_lcd_info));
	struct fb_info fbinfo;
	char *lcdname, *lcdparam, cmd[MAX_STRING];
	int nlcd, i, match;
	int next_option;
	const char*const short_options ="ci:r:";
	
	LOG("%s\n", __FUNCTION__);

	optind=0;
	while((next_option =getopt(argc,argv,short_options))!=-1) {
		switch (next_option){
		case 'c':
			mode = LCDMODE_SETCUR;
			break;
		case 'i':
			mode = LCDMODE_SETINDEX;
			i = strtol(optarg, NULL, 0);
			break; 
		case 'r':
			mode = LCDMODE_RM;
			lcdname = optarg;
			lcdparam = "";
			break; 
		case -1:	//Done with options.
			break; 
		default:
			FAILED_OUT("arguments error");
			return;
		}
	}

	match = em6000_load_lcds(&nlcd, plcdinfo, &fbinfo);

	if(mode == LCDMODE_SETINDEX){
		if(match!=i){
			if(i>=nlcd){
				FAILED_OUT("out of lcd list range");
				goto out;
			}
			if(set_lcd_info(&plcdinfo[i].info)<0)
				goto out;
		}
		SUCESS_OUT();
		goto out;
	}

	if(mode == LCDMODE_SET || mode == LCDMODE_SETCUR){
		if(argc < optind+2){
			FAILED_OUT("too few arguments");
			goto out;
		}
		lcdname = argv[optind];
		lcdparam = argv[optind+1];

		if(str2fbinfo(lcdparam, &fbinfo)<0){
			FAILED_OUT("lcd parameter format error");
			goto out;
		}

		if(run_cmd_quiet(NULL, NULL, 
			"sed -e 's/^[[:space:]]*//g' "LCD_LIST_FILE" | grep \'^\\<%s\\>\'>/dev/null && \\"
			"sed -i -e 's/^[[:space:]]*//g;s/^\\<%s\\>.*$/%s/g' "LCD_LIST_FILE" || \\"
			"echo %s >> "LCD_LIST_FILE, lcdname, lcdname, lcdparam, lcdparam)<0)
			goto out;

		if(mode == LCDMODE_SETCUR){
			if(set_lcd_info(&fbinfo)<0)
				goto out;
		}

		SUCESS_OUT();
		return;
	}

	if(mode == LCDMODE_RM){
		if(strcmp(plcdinfo[match].name,lcdname) == 0){
			FAILED_OUT("can't delete current lcd");
			goto out;
		}
		if(run_cmd_quiet(NULL, NULL, 
			"sed -i -e \'s/^[[:space:]]*//g;/^<\\%s\\>/d\' "LCD_LIST_FILE, lcdname)<0)
			goto out;

		SUCESS_OUT();
	}

out:
	free(plcdinfo);
}
BUILDIN_CMD("set-lcd", em6000_set_lcd, match_em6000);

