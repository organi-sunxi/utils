#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <linux/if.h>
#include "operator_machine.h"

void get_mac(int argc, char *argv[])
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

void set_mac(int argc, char *argv[])
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

void get_ip(int argc, char *argv[])
{
	/*
	struct ifreq ifr_ip;
	struct sockaddr_in *sin;
//	char ipaddr[50];
//	unsigned long ip;
	int res;

	if ((res = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	strcpy(ifr_ip.ifr_name, "eth0");
	if (ioctl(res, SIOCGIFADDR, &ifr_ip) < 0) {
		close(res);
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	sin = (struct sockaddr_in *)&(ifr_ip.ifr_addr);
//	strcpy(ipaddr, inet_ntoa(sin->sin_addr));

	printf("ip:%s\n", inet_ntoa(sin->sin_addr));

	close(res);
	*/
}

void set_ip(int argc, char *argv[])
{
	/*
	struct sockaddr_in sin;
	struct ifreq ifr_ip;
	int res;


	if ((res = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	memset(&sin, 0, sizeof(sin));
	strcpy(ifr_ip.ifr_name, "eth0");

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("192.168.1.200");
	memcpy(&ifr_ip.ifr_addr, &sin, sizeof(struct sockaddr_in));

	if (ioctl(res, SIOCSIFADDR, &ifr_ip) < 0) {
		close(res);
		printf("failed[%s]\n", strerror(errno));
		return;
	}
							
	ifr_ip.ifr_flags |= IFF_UP | IFF_RUNNING;
	if (ioctl(res, SIOCSIFFLAGS, &ifr_ip) < 0) {
		close(res);
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	close(res);
	printf("sucess\n");
	*/
}

void get_lcd(int argc, char *argv[])
{
	printf("get lcd\n");
}

void set_lcd(int argc, char *argv[])
{
	printf("set lcd\n");
}

void list_lcd(int argc, char *argv[])
{
	printf("list lcd\n");
}

void get_rotation(int argc, char *argv[])
{
	printf("get rotation\n");
}

void set_rotation(int argc, char *argv[])
{
	printf("set rotation\n");
}

void get_splash(int argc, char *argv[])
{
	printf("get splash\n");
}

void set_splash(int argc, char *argv[])
{
	printf("set splash\n");
}

void get_date(int argc, char *argv[])
{
	/*
	time_t timep;
	struct tm *p_tm = NULL;
	char strtime[15];

	timep = time(NULL);
	p_tm = localtime(&timep);

	strftime(strtime, sizeof(strtime), "%Y%m%d%H%M%S", p_tm);
	printf("%s\n", strtime);
	*/
}

void set_date(int argc, char *argv[])
{
	/*
	time_t timep;
	struct tm p_tm;
	struct timeval tv;

	memset(&p_tm, 0, sizeof(struct tm));

	p_tm.tm_sec = 0;
	p_tm.tm_min = 15;
	p_tm.tm_hour = 11;
	p_tm.tm_mday = 26;
	p_tm.tm_mon = 10 - 1;
	p_tm.tm_year = 2013 - 1900;

	if ((timep = mktime(&p_tm)) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	tv.tv_sec = timep;
	tv.tv_usec = 0;
	if (settimeofday(&tv, (struct timezone*)0) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	printf("success\n");
	*/
}

void get_host(int argc, char *argv[])
{
/*
	char hostname[50];

	if (gethostname(hostname, sizeof(hostname)) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}

	printf("%s\n", hostname);	
*/
}

void set_host(int argc, char *argv[])
{
/*
	if (sethostname(parameter, strlen(parameter)) < 0) {
		printf("failed[%s]\n", strerror(errno));
		return;
	}
*/
	printf("success\n");
}
