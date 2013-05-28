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

static char s_lcdtimings_buf[MAX_STRING*50];
static int s_lcdindex;
struct lcdinfo{
	char *lcdname;
	char *lcdparam;
};

static void get_all_lcds(char out[], int n)
{
	strncat(s_lcdtimings_buf, out, sizeof(s_lcdtimings_buf));
}


#define STATE_FIND_LCDNAME	0
#define STATE_LCDNAME		1
#define STATE_FIND_LCDPARAM	2
#define STATE_LCDPARAM		3

static int parse_all_lcds(int *n, struct lcdinfo *info)
{
	char *ch = s_lcdtimings_buf;
	int state = STATE_FIND_LCDNAME, ret;
	int len;

	*ch = 0;
	ret = run_cmd_quiet(get_all_lcds, "%s", "fw_printenv lcdtimings");
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

static void lcd_index_content(char out[], int n)
{
	s_lcdindex = -1;
	sscanf(out, "lcdindex=%d\n", &s_lcdindex);
}

static int get_lcd_index()
{
	if (run_cmd_quiet(lcd_index_content, "%s", "fw_printenv lcdindex") < 0) {
		return -1;
	}

	return s_lcdindex;
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

#define LCDMODE_SET			0
#define LCDMODE_SETCUR		1
#define LCDMODE_SETINDEX	2
#define LCDMODE_RM			3

static void set_lcd(int argc, char *argv[])
{
	struct lcdinfo lcdinfo[MAX_ARGS];
	char *setcmd, *lcdname=NULL, *lcdparam=NULL, *p;
	int buf_size, nlcd, mode=LCDMODE_SET, i, mach=0;
	int next_option;
	const char*const short_options ="ci:r:";
	
	LOG("%s\n", __FUNCTION__);

	optind=1;

	do {
		next_option =getopt(argc,argv,short_options); 
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
	}while (next_option !=-1); 

	if(mode == LCDMODE_SETINDEX){
		if(run_cmd_quiet(NULL, "fw_setenv lcdindex %d", i)==0)
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
					run_cmd_quiet(NULL, "fw_setenv lcdindex %d", index-1);
				}
				continue;
			}

			p += sprintf(p, "%s %s\n", lcdname, lcdparam);
			if (mode == LCDMODE_SETCUR){
				if(run_cmd_quiet(NULL, "fw_setenv lcdindex %d", i)<0)
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
				if(run_cmd_quiet(NULL, "fw_setenv lcdindex %d", i)<0)
					goto failed1;
			}
		}
	}

	if(run_cmd_quiet(NULL, "%s\"", setcmd)<0)
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
	int ret;

	LOG("%s\n", __FUNCTION__);
	ret = get_conf_file(GET_CONF_VALUE(SYS_CONF), "ROTATE", value);

	if(ret<=0){
		printf("0\n");
		return;
	}

	if(strncmp(value, "Transformed:Rot:", 16)==0){
		printf("%s\n", value+16);
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
		snprintf(value, sizeof(value), "Transformed:Rot:%d", v);
		break;
	default:
		FAILED_OUT("error angel");
		return;
	}

	set_conf_file(GET_CONF_VALUE(SYS_CONF), "ROTATE", value);
	SUCESS_OUT();
}
BUILDIN_CMD("set-rotation", set_rotation);

static void get_splash(int argc, char *argv[])
{
	printf("get splash\n");
}
BUILDIN_CMD("get-splash", get_splash);
	
static void set_splash(int argc, char *argv[])
{
	printf("set splash\n");
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
		FAILED_OUT("too few arguments to command 'set-date'");
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

