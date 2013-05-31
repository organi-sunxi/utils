#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <linux/fb.h>

#include "lib.h"
#include "command.h"
#include "logmessage.h"

static int match_em5000(const char *platform)
{
	return strncmp(platform, "EM5", 3)==0;
}

struct lcdinfo{
	char *lcdname;
	char *lcdparam;
};

static void get_all_lcds(char *buf, char out[], int n)
{
	strcat(buf, out);
}

#define STATE_FIND_LCDNAME	0
#define STATE_LCDNAME		1
#define STATE_FIND_LCDPARAM	2
#define STATE_LCDPARAM		3

static int parse_all_lcds(int *n, struct lcdinfo *info)
{
	static char s_lcdtimings_buf[MAX_STRING*50];
	char *ch = s_lcdtimings_buf;
	int state = STATE_FIND_LCDNAME, ret;
	int len;

	*ch = 0;
	ret = run_cmd_quiet((out_callback_fun)get_all_lcds, ch, "%s", "fw_printenv lcdtimings");
	if ( ret < 0)
		return ret;

	len = strlen(s_lcdtimings_buf);

	if (strncmp(s_lcdtimings_buf, "lcdtimings=", 11) == 0) {
		ch = s_lcdtimings_buf + 11;
	}
	else{
		FAILED_OUT("can't find lcd timings");
		return -1;
	}

	for (*n = 0; *ch != '\0'; ch++) {
		if (*ch == '\n') {
			*ch = 0;
			state = STATE_FIND_LCDNAME;
			continue;
		}

		switch(state){
		case STATE_FIND_LCDNAME:
			if(*ch!=' '){
				state = STATE_LCDNAME;
				info->lcdname = ch;
			}
			break;
		case STATE_LCDNAME:
			if(*ch==' '){
				state = STATE_FIND_LCDPARAM;
				*ch = 0;
			}
			break;
		case STATE_FIND_LCDPARAM:
			if(*ch!=' '){
				state = STATE_LCDPARAM;
				info->lcdparam = ch;
				(*n)++;
				info++;
			}
			break;
		}
	}

	return len;
}

static void lcd_index_content(int *lcdindex, char out[], int n)
{
	sscanf(out, "lcdindex=%d\n", lcdindex);
}

static int get_lcd_index()
{
	int lcdindex = -1;

	if (run_cmd_quiet((out_callback_fun)lcd_index_content, &lcdindex, "%s", "fw_printenv lcdindex") < 0) {
		return -1;
	}

	return lcdindex;
}

static void get_lcd(int argc, char *argv[])
{
	struct lcdinfo lcdinfo[MAX_ARGS];
	int nlcd;
	int index = -1;
	
	LOG("%s\n", __FUNCTION__);

	index = get_lcd_index();

	if(parse_all_lcds(&nlcd, lcdinfo)<0)
		return;
	
	if ((index >= 0) && (index < nlcd)) {
		printf("%s %s\n", lcdinfo[index].lcdname, lcdinfo[index].lcdparam);
		return;
	}

	FAILED_OUT("error lcd index");
}
BUILDIN_CMD("get-lcd", get_lcd, match_em5000);


#define set_lcd_index(i) run_cmd_quiet(NULL, NULL, "fw_setenv lcdindex %d", i)
static void set_lcd(int argc, char *argv[])
{
	enum lcdinfo_mode{
		LCDMODE_SET,
		LCDMODE_SETCUR,
		LCDMODE_SETINDEX,
		LCDMODE_RM,
	}mode = LCDMODE_SET;

	struct lcdinfo lcdinfo[MAX_ARGS];
	char *setcmd, *lcdname=NULL, *lcdparam=NULL, *p;
	int buf_size, nlcd, i, mach=0;
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

	if(mode == LCDMODE_SETINDEX){
		if(set_lcd_index(i)==0)
			SUCESS_OUT();
		return;
	}

	if(mode == LCDMODE_SET || mode == LCDMODE_SETCUR){
		if(argc < optind+2){
			FAILED_OUT("too few arguments");
			return;
		}
		lcdname = argv[optind];
		lcdparam = argv[optind+1];
	}

	buf_size = parse_all_lcds(&nlcd, lcdinfo);
	if(buf_size<=0)
		return;

	buf_size += strlen(lcdname) + strlen(lcdparam) + 10;
	p = setcmd = (char *)malloc(buf_size);
	p += sprintf(p, "fw_setenv lcdtimings \"");
	
	for (i = 0; i < nlcd; i++){
		if (strcmp(lcdinfo[i].lcdname, lcdname) == 0 ){
			mach = 1;
			if(mode == LCDMODE_RM){
				int index = get_lcd_index();
				if(i<index){
					set_lcd_index(index-1);
				}
				continue;
			}

			p += sprintf(p, "%s %s\n", lcdname, lcdparam);
			if (mode == LCDMODE_SETCUR){
				if(set_lcd_index(i)<0)
					goto failed1;
			}
			continue;
		}

		p += sprintf(p, "%s %s\n", lcdinfo[i].lcdname, lcdinfo[i].lcdparam);
	}

	if(!mach){
		if(mode == LCDMODE_RM){
			FAILED_OUT("can't find lcd %s to delete", lcdname);
			goto failed1;
		}
		else{
			p += sprintf(p, "%s %s\n", lcdname, lcdparam);
			if (mode == LCDMODE_SETCUR){
				if(set_lcd_index(i)<0)
					goto failed1;
			}
		}
	}

	if(run_cmd_quiet(NULL, NULL, "%s\"", setcmd)<0)
		goto failed1;

	SUCESS_OUT();
failed1:
	free(setcmd);
}
BUILDIN_CMD("set-lcd", set_lcd, match_em5000);

static void list_lcd(int argc, char *argv[])
{
	struct lcdinfo lcdinfo[MAX_ARGS];
	int nlcd, i;
	int index = -1;
	
	LOG("%s\n", __FUNCTION__);

	index = get_lcd_index();

	parse_all_lcds(&nlcd, lcdinfo);

	for (i=0; i <nlcd; i++) {
		printf("%c%s %s\n", i==index ? '*':' ', lcdinfo[i].lcdname, lcdinfo[i].lcdparam);
	}
}
BUILDIN_CMD("list-lcd", list_lcd, match_em5000);

static void splash_size_content(int *size, char out[], int n)
{
	if(sscanf(out, "splashsize=%x\n", size)!=1)
		FAILED_OUT("get size error");
}

static int get_splash_size()
{
	int size = -1;

	if (run_cmd_quiet((out_callback_fun)splash_size_content, &size, "%s", "fw_printenv splashsize") < 0) {
		return -1;
	}

	return size;
}

static void get_splash(int argc, char *argv[])
{
	struct fb_info fbinfo;
	const char *tmppath = GET_CONF_VALUE(TMPFILE_PATH);
	int size, ret;

	LOG("%s\n", __FUNCTION__);
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	size = get_splash_size();
	if(size<=0)
		return;

	ret = run_cmd_quiet(NULL, NULL, 
		"dd if=%s of=%s/splash.bin count=1 bs=%d 2>/dev/null", 
		GET_CONF_VALUE(SPLASH_DEVICE), tmppath, size);
	if(ret<0)
		return;

	if(get_fb_info(&fbinfo)<0)
		return;

	ret = run_cmd_quiet(NULL, NULL, "mksplash -t %s/tmpfile -rd -b%d -o %s %s/splash.bin", 
		tmppath, fbinfo.bpp, argv[1], tmppath);
	if(ret<0)
		return;

	SUCESS_OUT();
}
BUILDIN_CMD("get-splash", get_splash, match_em5000);

static void set_splash(int argc, char *argv[])
{
	struct fb_info fbinfo;
	const char *tmppath = GET_CONF_VALUE(TMPFILE_PATH), *splashdev=GET_CONF_VALUE(SPLASH_DEVICE);
	char cmd[MAX_STRING];
	int ret;
	unsigned long size;

	LOG("%s\n", __FUNCTION__);
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	if(get_fb_info(&fbinfo)<0)
		return;

	ret = run_cmd_quiet(NULL, NULL, "mksplash -t %s/tmpfile -d -b%d -o %s/splash.bin %s", 
		tmppath, fbinfo.bpp, tmppath, argv[1]);
	if(ret<0)
		return;

	snprintf(cmd, sizeof(cmd), "%s/splash.bin", tmppath);
	size = get_file_size(cmd);
	if(size==0){
		FAILED_OUT("make splash error");
		return;
	}

	ret = run_cmd_quiet(NULL, NULL, 
		"flash_eraseall %s && nandwrite -p %s %s/splash.bin && fw_setenv splashsize %x", 
		splashdev, splashdev, tmppath, size);
	if(ret<0){
		FAILED_OUT("write splash error");
		return;
	}

	SUCESS_OUT();
}
BUILDIN_CMD("set-splash", set_splash, match_em5000);
