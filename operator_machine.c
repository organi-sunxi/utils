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
#include "command.h"
#include "logmessage.h"

#define MAX_STRING 256
#define MAX_ARGS 50

static void get_mac(int argc, char *argv[])
{
	struct ifreq ifr_mac;
	struct ifreq *tmp = NULL;
	const char *eth = GET_CONF_VALUE(ETH_NAME);
	unsigned char mac[6];
	int res;

	LOG("%s\n", __FUNCTION__);
	
	if ((res = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		FAILED_OUT(strerror(errno));
		return;
	}

	strcpy(ifr_mac.ifr_name, eth);
	
	if (ioctl(res, SIOCGIFHWADDR, &ifr_mac) < 0) {
		close(res);
		FAILED_OUT(strerror(errno));
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
		FAILED_OUT(strerror(errno));
		return;
	}

	strcpy(ifr_ip.ifr_name, eth);
	if (ioctl(res, SIOCGIFADDR, &ifr_ip) < 0) {
		close(res);
		FAILED_OUT(strerror(errno));
		return;
	}

	sin = (struct sockaddr_in *)&(ifr_ip.ifr_addr);

	printf("%s\n", inet_ntoa(sin->sin_addr));

	close(res);
}
BUILDIN_CMD("get-ip", get_ip);

static void set_ip(int argc, char *argv[])
{
	const char *sysconf_path = GET_CONF_VALUE(SYS_CONF);
	char sysconf_line[MAX_STRING];
	char *sysconf_content = NULL;
	long sysconf_size;
	FILE *res = NULL;

	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'set-ip'");
		return;
	}

	if (!(res = fopen(sysconf_path, "r+"))) {
		FAILED_OUT(strerror(errno));
		return;
	}

	fseek(res, 0, SEEK_END);
	sysconf_size = ftell(res);
	sysconf_content = (char *)malloc(sysconf_size + MAX_STRING + 1);
	memset(sysconf_content, 0, sysconf_size + MAX_STRING + 1);	
	fseek(res, 0, SEEK_SET);

	while (fgets(sysconf_line, sizeof(sysconf_line), res)) {
		if (strncmp(sysconf_line, "IPADDR", 6) == 0) {
			memset(sysconf_line, 0, sizeof(sysconf_line));
			strcpy(sysconf_line, "IPADDR=");
			strcat(sysconf_line, argv[argc - 1]);
			strcat(sysconf_line, "\n");
		}

		strcat(sysconf_content, sysconf_line);
	}
	fclose(res);
	
	res = fopen(sysconf_path, "w");
	fputs(sysconf_content, res);
	fclose(res);
	
	free(sysconf_content);
	SUCESS_OUT();
}
BUILDIN_CMD("set-ip", set_ip);

#define STATE_WHITESPACE (0)
#define STATE_WORD (1)
static void parse_all_lcds(char *lcdbuf, int *argc, char **argv)
{
	char *ch = lcdbuf;
	int state = STATE_WHITESPACE;

	argv[0]=ch;
	if (strncmp(lcdbuf, "lcdtimings=", 11) == 0) {
		strcpy(lcdbuf, lcdbuf + 11);	
	}

	for (*argc = 0; *ch != '\0'; ch++) {
		if (*ch == '\n') {
			*ch = '\0';
			state = STATE_WHITESPACE;
			continue;
		}

		if (state == STATE_WHITESPACE){
			argv[*argc] = ch;
			(*argc)++;
			state = STATE_WORD;
		}
	}
}

static int get_lcd_index()
{
	char buf[MAX_STRING];
	FILE *res = popen("fw_printenv lcdindex", "r");

	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), res);
	if (strncmp(buf, "lcdindex=", 9) == 0) {
		return atoi(&buf[9]);
	}

	return -1;
}

static void get_lcd(int argc, char *argv[])
{
	char buf[MAX_STRING];
	char *lcd_argv[MAX_ARGS];
	int lcd_argc;
	int index = -1;
	FILE *res;
	
	LOG("%s\n", __FUNCTION__);

	index = get_lcd_index();

	res = popen("fw_printenv lcdtimings", "r");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), res);
	
	memset(lcd_argv, 0, sizeof(lcd_argv));
	parse_all_lcds(buf, &lcd_argc, lcd_argv);
	
	if ((index >= 0) && (index < lcd_argc)) {
		printf("%s\n", lcd_argv[index]);
		return;
	}

	FAILED_OUT("indexlcd is error");
}
BUILDIN_CMD("get-lcd", get_lcd);

static void set_lcd(int argc, char *argv[])
{
	char buf[MAX_STRING];
	char *lcd_argv[MAX_ARGS];
	int lcd_argc;
	int index = -1;
/*	
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments to function 'set-date'");
		return;
	}*/

	system("fw_setenv lcdtimings sharp_lq057 640x480@23500,48:32:80,15:15:15,BODPhv\
			\nAUO_G070VW01V0 800x480@29500,10:192:10,10:20:10,BODPhv\
			\njoe 800x480@29500,10:192:10,10:20:10,BODPhv\n");
}
BUILDIN_CMD("set-lcd", set_lcd);

static void list_lcd(int argc, char *argv[])
{
	char buf[MAX_STRING];
	char *lcd_argv[MAX_ARGS];
	int lcd_argc;
	int index = -1;
	FILE *res;
	
	LOG("%s\n", __FUNCTION__);

	index = get_lcd_index();

	res = popen("fw_printenv lcdtimings", "r");
	memset(buf, 0, sizeof(buf));
	fread(buf, sizeof(char), sizeof(buf), res);

	memset(lcd_argv, 0, sizeof(lcd_argv));
	parse_all_lcds(buf, &lcd_argc, lcd_argv);

	while (lcd_argc--) {
		if (lcd_argc != index) {
			printf(" %s\n", lcd_argv[lcd_argc]);
		}
		else {
			printf("*%s\n", lcd_argv[lcd_argc]);
		}
	}
}
BUILDIN_CMD("list-lcd", list_lcd);

static void get_rotation(int argc, char *argv[])
{
	const char *sysconf_path = GET_CONF_VALUE(SYS_CONF);
	char sysconf_line[MAX_STRING];
	FILE *res = NULL;

	LOG("%s\n", __FUNCTION__);

	if (!(res = fopen(sysconf_path, "r"))) {
		FAILED_OUT(strerror(errno));
		return;
	}

	while (fgets(sysconf_line, sizeof(sysconf_line), res)) {
		if (strncmp(sysconf_line, "ROTATE=Transformed:Rot", 22) == 0) {
			strcpy(sysconf_line, &sysconf_line[22]);
			printf("%d\n", atoi(sysconf_line));
			break;
		}

		memset(sysconf_line, 0, sizeof(sysconf_line));
	}
	
	fclose(res);
}
BUILDIN_CMD("get-rotation", get_rotation);

static void set_rotation(int argc, char *argv[])
{
	const char *sysconf_path = GET_CONF_VALUE(SYS_CONF);
	char sysconf_line[MAX_STRING];
	char *sysconf_content = NULL;
	long sysconf_size;
	FILE *res = NULL;

	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'set-ip'");
		return;
	}

	if (!(res = fopen(sysconf_path, "r+"))) {
		FAILED_OUT(strerror(errno));
		return;
	}

	fseek(res, 0, SEEK_END);
	sysconf_size = ftell(res);
	sysconf_content = (char *)malloc(sysconf_size + MAX_STRING + 1);
	memset(sysconf_content, 0, sysconf_size + MAX_STRING + 1);	
	fseek(res, 0, SEEK_SET);

	while (fgets(sysconf_line, sizeof(sysconf_line), res)) {
		if (strncmp(sysconf_line, "ROTATE=Transformed:Rot", 22) == 0) {
			memset(sysconf_line, 0, sizeof(sysconf_line));
			strcpy(sysconf_line, "ROTATE=Transformed:Rot");
			strcat(sysconf_line, argv[argc - 1]);
			strcat(sysconf_line, ":\n");
		}

		strcat(sysconf_content, sysconf_line);
	}
	fclose(res);
	
	res = fopen(sysconf_path, "w");
	fputs(sysconf_content, res);
	fclose(res);
	
	free(sysconf_content);
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
		FAILED_OUT(strerror(errno));
		return;
	}

	tv.tv_sec = timep;
	tv.tv_usec = 0;
	if (settimeofday(&tv, (struct timezone*)0) < 0) {
		FAILED_OUT(strerror(errno));
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
		FAILED_OUT(strerror(errno));
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
		FAILED_OUT(strerror(errno));
		return;
	}

	fputs(argv[argc - 1], res);
	fclose(res);

	SUCESS_OUT();
}
BUILDIN_CMD("set-host", set_host);

