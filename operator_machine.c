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

#define MAX_STRING 256
#define MAX_FILE_LEN 1024

static void get_mac(int argc, char *argv[])
{
	printf("get_mac\n");
#if 0
	struct ifreq ifr_mac;
	struct ifreq *tmp = NULL;
//	struct ifconf ifc;
	int res, i;
//	char buf[1024];
	unsigned char mac[6];

	if ((res = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}


	strcpy(ifr_mac.ifr_name, "eth0");
	
	if (ioctl(res, SIOCGIFHWADDR, &ifr_mac) < 0) {
		close(res);
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	bcopy(&ifr_mac.ifr_hwaddr.sa_data, mac, sizeof(mac));
	printf("%s: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
			ifr_mac.ifr_name,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	/*
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	ioctl(res, SIOCGIFCONF, &ifc);
	tmp = ifc.ifc_req;

	for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; tmp++) {
		if (!strncmp(tmp->ifr_name, "eth", 3)) {
			strcpy(ifr.ifr_name, tmp->ifr_name);

			if (!ioctl(res, SIOCGIFHWADDR, &ifr)) {
				memset(mac, 0, sizeof(mac));
				bcopy(ifr.ifr_hwaddr.sa_data, mac, 6);

				printf("%s: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n", 
						ifr.ifr_name,
						mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			}
		}
	}
	*/

	close(res);
#endif
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
	char sysconf_content[MAX_FILE_LEN] = {'\0'};
	FILE *res = NULL;

	if (argc < 2) {
		FAILED_OUT("too few arguments to function 'set-ip'");
		return;
	}

	if (!(res = fopen(sysconf_path, "r+"))) {
		FAILED_OUT(strerror(errno));
		return;
	}

	while (fgets(sysconf_line, sizeof(sysconf_line), res)) {
		if (strncmp(sysconf_line, "IPADDR", 6) == 0) {
			memset(sysconf_line, 0, sizeof(sysconf_line));
			strcpy(sysconf_line, "IPADDR=");
			strcat(sysconf_line, argv[argc - 1]);
			strcat(sysconf_line, "\n");
		}

		strcat(sysconf_content, sysconf_line);
	}

	fseek(res, 0, SEEK_SET);
	fputs(sysconf_content, res);
	fclose(res);
	SUCESS_OUT();
}
BUILDIN_CMD("set-ip", set_ip);

static void get_lcd(int argc, char *argv[])
{
	printf("get lcd\n");
}
BUILDIN_CMD("get-lcd", get_lcd);

static void set_lcd(int argc, char *argv[])
{
	printf("set lcd\n");
}
BUILDIN_CMD("set-lcd", set_lcd);

static void list_lcd(int argc, char *argv[])
{
	printf("list lcd\n");
}
BUILDIN_CMD("list-lcd", list_lcd);

static void get_rotation(int argc, char *argv[])
{
	printf("get rotation\n");
}
BUILDIN_CMD("get-rotation", get_rotation);

static void set_rotation(int argc, char *argv[])
{
	printf("set rotation\n");
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

	if (argc < 2) {
		FAILED_OUT("too few arguments to function 'set-date'");
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
	
	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'set-host'");
		return;
	}

	if (!(res = fopen(host_path, "w+"))) {
		FAILED_OUT(strerror(errno));
		return;
	}

	fputs(argv[argc - 1], res);
	fclose(res);

	SUCESS_OUT();
}
BUILDIN_CMD("set-host", set_host);

