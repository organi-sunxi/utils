#include <stdio.h>
#include <stdlib.h>

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

//return match lcdindex or -1
static int em6000_load_lcds(int *nlcd, struct local_lcd_info *lcdinfo, struct fb_info *fbinfo)
{
	FILE *pf;
	char buf[MAX_STRING], suffix[128], *p;
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

		//[width]x[height]@[PCLK(KHz)],[HFP]:[HSW]:[HBP],[VFP]:[VSW]:[VBP],BODPHV
		ret = sscanf(buf, "%s%dx%d@%d,%d:%d:%d,%d:%d:%d%[^#]",lcdinfo->name, 
			&lcdinfo->info.width, &lcdinfo->info.height, &lcdinfo->info.pclk, 
			&lcdinfo->info.hfp, &lcdinfo->info.hsw, &lcdinfo->info.hbp,
			&lcdinfo->info.vfp, &lcdinfo->info.vsw, &lcdinfo->info.vbp, suffix);
		if(ret<10)
			continue;

		if(ret==10)
			lcdinfo->info.pol=0;
		else
			get_lcd_pol(suffix, &lcdinfo->info.pol);

		if(!fbinfo)
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

