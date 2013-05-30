#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <linux/if.h>
#include <getopt.h>
#include <linux/fb.h>
#include <fcntl.h>

#include "command.h"
#include "logmessage.h"

#define MAX_ARGS 50

static void platform(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);
	
	printf("%s\n", GET_CONF_VALUE(PLATFORM));
}
BUILDIN_CMD("platform", platform);

static void get_mac(int argc, char *argv[])
{
	struct ifreq ifr_mac;
	struct ifreq *tmp = NULL;
	const char *eth = GET_CONF_VALUE(ETH_NAME);
	unsigned char mac[6];
	int res;

	LOG("%s\n", __FUNCTION__);
	
	if ((res = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	strcpy(ifr_mac.ifr_name, eth);
	
	if (ioctl(res, SIOCGIFHWADDR, &ifr_mac) < 0) {
		close(res);
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	bcopy(&ifr_mac.ifr_hwaddr.sa_data, mac, sizeof(mac));
	printf("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	close(res);
}
BUILDIN_CMD("get-mac", get_mac);

static void set_mac(int argc, char *argv[])
{
	FAILED_OUT("not support");
#if 0
	struct ifreq ifr_mac;
//	struct ifreq *tmp = NULL;
//	struct ifconf ifc;
	int res;
//	char buf[1024];
	uint8_t mac[6];

		

	mac[0] = 0xAA;
	mac[1] = 0xBB;
	mac[2] = 0xCC;
	mac[3] = 0xDD;
	mac[4] = 0xEE;
	mac[5] = 0xFF;

	if ((res = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	ifr_mac.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	strcpy(ifr_mac.ifr_name, "eth0");
	//bcopy(mac, &ifr_mac.ifr_hwaddr.sa_data, IFHWADDRLEN);
	memcpy(&ifr_mac.ifr_hwaddr.sa_data, mac, IFHWADDRLEN);

	if (ioctl(res, SIOCSIFHWADDR, &ifr_mac) < 0) {
		close(res);
		printf("failed1[%s]\n", strerror(errno));
		return;
	}
/*
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	ioctl(res, SIOCGIFCONF, &ifc);
	tmp = ifc.ifc_req;

	for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; tmp++) {
		if (!strncmp(tmp->ifr_name, "eth", 3)) {
			printf("name:%s\n", tmp->ifr_name);
			strcpy(ifr.ifr_name, tmp->ifr_name);
			memcpy(&ifr.ifr_hwaddr.sa_data, mac, 6);			
			if (ioctl(res, SIOCSIFHWADDR, &ifr) < 0) {
				close(res);
				perror("SIOCSIFHWADDR");
				return;
			}
		}
	}
*/	
	close(res);
	printf("sucess\n");
#endif
}
BUILDIN_CMD("set-mac", set_mac);

static void get_ip(int argc, char *argv[])
{
	struct ifreq ifr_ip;
	struct sockaddr_in *sin;
	const char *eth = GET_CONF_VALUE(ETH_NAME);
	int res;

	LOG("%s\n", __FUNCTION__);
	
	if ((res = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	strcpy(ifr_ip.ifr_name, eth);
	if (ioctl(res, SIOCGIFADDR, &ifr_ip) < 0) {
		close(res);
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	sin = (struct sockaddr_in *)&(ifr_ip.ifr_addr);

	printf("%s\n", inet_ntoa(sin->sin_addr));

	close(res);
}
BUILDIN_CMD("get-ip", get_ip);

static void set_ip(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'set-ip'");
		return;
	}

	if(set_conf_file(GET_CONF_VALUE(SYS_CONF), "IPADDR", argv[1])==0)
		SUCESS_OUT();
}
BUILDIN_CMD("set-ip", set_ip);

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
BUILDIN_CMD("get-lcd", get_lcd);


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
BUILDIN_CMD("set-lcd", set_lcd);

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
BUILDIN_CMD("list-lcd", list_lcd);

static void get_rotation(int argc, char *argv[])
{
	char value[MAX_STRING];
	FILE *res = NULL;
	int ret, a;

	LOG("%s\n", __FUNCTION__);
	ret = get_conf_file(GET_CONF_VALUE(SYS_CONF), "ROTATE", value);

	if(ret<=0){
		printf("0\n");
		return;
	}

	ret = sscanf(value, "Transformed:Rot%d:", &a);
	if(ret ==1){
		printf("%d\n", a);
		return;
	}

	FAILED_OUT("error format");
}
BUILDIN_CMD("get-rotation", get_rotation);

static void set_rotation(int argc, char *argv[])
{
	char value[MAX_STRING];
	int v;
	FILE *res = NULL;

	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	v = strtol(argv[1], NULL, 0);
	switch(v){
	case 0:
		value[0] = 0;
		break;
	case 90:
	case 180:
	case 270:
		snprintf(value, sizeof(value), "Transformed:Rot%d:", v);
		break;
	default:
		FAILED_OUT("error angle");
		return;
	}

	if(set_conf_file(GET_CONF_VALUE(SYS_CONF), "ROTATE", value)<0)
		return;

	SUCESS_OUT();
}
BUILDIN_CMD("set-rotation", set_rotation);

static void get_bklight(int argc, char *argv[])
{
	char file[MAX_STRING], value[MAX_STRING];
	int v, max;

	LOG("%s\n", __FUNCTION__);

	snprintf(file, sizeof(file), "%s/%s", GET_CONF_VALUE(SYSPATH_BKLIGHT), "brightness");
	if(read_device_line(file, value, sizeof(value))<0)
		return;
	v = atoi(value);

	snprintf(file, sizeof(file), "%s/%s", GET_CONF_VALUE(SYSPATH_BKLIGHT), "max_brightness");
	if(read_device_line(file, value, sizeof(value))<0)
		return;

	max = atoi(value);
	if(max<=0){
		FAILED_OUT("max brightness error");
		return;
	}

	printf("%d\n", v*100/max);
}
BUILDIN_CMD("get-bklight", get_bklight);

static void set_bklight(int argc, char *argv[])
{
	char file[MAX_STRING], value[MAX_STRING];
	int next_option;
	const char*const short_options ="s";
	int v, max, save=0;

	LOG("%s\n", __FUNCTION__);

	snprintf(file, sizeof(file), "%s/%s", GET_CONF_VALUE(SYSPATH_BKLIGHT), "max_brightness");
	if(read_device_line(file, value, sizeof(value))<0)
		return;

	max = atoi(value);
	if(max<=0){
		FAILED_OUT("max brightness error");
		return;
	}

	optind=0;
	while((next_option =getopt(argc,argv,short_options))!=-1) {
		switch (next_option){
		case 's':
			save = 1;
			break;
		case -1:	//Done with options.
			break; 
		default:
			FAILED_OUT("arguments error");
			return;
		}
	}

	if(argc < optind+1){
		if(save){
			snprintf(file, sizeof(file), "%s/%s", GET_CONF_VALUE(SYSPATH_BKLIGHT), "brightness");
			if(read_device_line(file, value, sizeof(value))<0)
				return;
			v = atoi(value);
		}
		else{
			FAILED_OUT("too few arguments");
			return;
		}
	}
	else{
		v = atoi(argv[optind]);
		v = v*max/100;
	}

	snprintf(file, sizeof(file), "%s/%s", GET_CONF_VALUE(SYSPATH_BKLIGHT), "brightness");
	snprintf(value, sizeof(value), "%d", v);
	if(write_device_line(file, value)<0)
		return;

	if(save){
		if(set_conf_file(GET_CONF_VALUE(SYS_CONF), "BACKLIGHT", value)<0)
			return;
	}
		
	SUCESS_OUT();
}
BUILDIN_CMD("set-bklight", set_bklight);

struct fb_info
{
	int width, height;
	int bpp;
};

static int get_fb_info(struct fb_info *fbinfo)
{
	int fb;
	
	struct fb_var_screeninfo fb_vinfo;

	fb = open (GET_CONF_VALUE(FRAMEBUFFER), O_RDWR);
	if(fb<0){
		FAILED_OUT("open fb device");
		return fb;
	}

	if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_vinfo)) {
		FAILED_OUT("Can't get VSCREENINFO\n");
		close(fb);
		return -1;
	}

	fbinfo->bpp= fb_vinfo.red.length + fb_vinfo.green.length +
                fb_vinfo.blue.length + fb_vinfo.transp.length;

	fbinfo->width = fb_vinfo.xres;
	fbinfo->height = fb_vinfo.yres;

	close(fb);
	return 0;
}

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
BUILDIN_CMD("get-splash", get_splash);

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
BUILDIN_CMD("set-splash", set_splash);

static void get_date(int argc, char *argv[])
{
	time_t timep;
	struct tm *p_tm = NULL;
	char strtime[MAX_STRING];

	LOG("%s\n", __FUNCTION__);
	
	timep = time(NULL);
	p_tm = localtime(&timep);

	strftime(strtime, sizeof(strtime), "%Y%m%d%H%M%S", p_tm);
	printf("%s\n", strtime);
}
BUILDIN_CMD("get-date", get_date);

static void set_date(int argc, char *argv[])
{
	time_t timep;
	struct tm p_tm;
	struct timeval tv;
	char *src = argv[argc - 1];
	char tmp[MAX_STRING];

	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	memset(&p_tm, 0, sizeof(struct tm));
	strcpy(src, argv[argc - 1]);

	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, src, 4);
	src += 4;
	p_tm.tm_year = atoi(tmp) -1900;

	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, src, 2);
	src += 2;
	p_tm.tm_mon = atoi(tmp) - 1;

	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, src, 2);
	src += 2;
	p_tm.tm_mday = atoi(tmp);

	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, src, 2);
	src += 2;
	p_tm.tm_hour = atoi(tmp);

	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, src, 2);
	src += 2;
	p_tm.tm_min = atoi(tmp);

	memset(tmp, 0, sizeof(tmp));
	strncpy(tmp, src, 2);
	p_tm.tm_sec = atoi(tmp);

	if ((timep = mktime(&p_tm)) < 0) {
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	tv.tv_sec = timep;
	tv.tv_usec = 0;
	if (settimeofday(&tv, (struct timezone*)0) < 0) {
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	system("hwclock -w");
	SUCESS_OUT();
}
BUILDIN_CMD("set-date", set_date);

static void get_host(int argc, char *argv[])
{
	char hostname[MAX_STRING];

	LOG("%s\n", __FUNCTION__);
	
	if (gethostname(hostname, sizeof(hostname)) < 0) {
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	printf("%s\n", hostname);
}
BUILDIN_CMD("get-host", get_host);

static void set_host(int argc, char *argv[])
{
	const char *host_path = GET_CONF_VALUE(HOSTNAME_FILE);
	FILE *res = NULL;

	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'set-host'");
		return;
	}

	if (!(res = fopen(host_path, "w"))) {
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	fputs(argv[argc - 1], res);
	fclose(res);

	SUCESS_OUT();
}
BUILDIN_CMD("set-host", set_host);

