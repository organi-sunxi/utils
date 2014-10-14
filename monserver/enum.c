#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#include <netdb.h>

#include "enum.h"

#define BUFFER_SIZE		1024

static unsigned long enum_CRC32(unsigned char *ptr, unsigned char len)
{	
	const unsigned long CRCtbl[16]={
		0x00000000L, 0x1db71064L, 0x3b6e20c8L, 0x26d930acL, 0x76dc4190L, 0x6b6b51f4L, 0x4db26158L, 0x5005713cL,
		0xedb88320L, 0xf00f9344L, 0xd6d6a3e8L, 0xcb61b38cL, 0x9b64c2b0L, 0x86d3d2d4L, 0xa00ae278L, 0xbdbdf21cL,};

	unsigned long crc=0xffffffff;
	unsigned char da;

	while(len--!=0){
		da = crc^*ptr;	 
		crc >>= 4;	 
		crc ^= CRCtbl[da&0x0f];   
		da = crc^((*ptr)>>4);	
		crc >>= 4;	 
		crc ^= CRCtbl[da&0x0f];   
		ptr++;	 
	}
	return(crc);
}  

static int check_crc(struct enumd_cmd_hd *hd, int size)
{
	int32_t crc;

	if(size<=ENUM_TOTAL_SIZE(hd))
		return -1;

	size = ENUM_CRC_OFFSET(hd);

	crc = enum_CRC32((unsigned char *)hd, size);
	return (crc == ENUM_CMD_CRC(hd));
}

static int list_cmd(struct sockaddr *src_addr, socklen_t sin_size, 
	int sendfd, struct enumd_cmd_hd *hd, uint8_t mac[6])
{
	struct enumd_devid *id = (struct enumd_devid *)hd->payload;

	gethostname(id->hostname, 
		BUFFER_SIZE-sizeof(struct enumd_cmd_hd)-sizeof(struct enumd_devid));
	memcpy(id->mac, mac, 6);
	hd->payload_size = 6+strlen(id->hostname)+1;

	ENUM_CMD_CRC(hd)=enum_CRC32((unsigned char *)hd, ENUM_CRC_OFFSET(hd));

	sendto(sendfd, hd, ENUM_TOTAL_SIZE(hd), 0, src_addr, sin_size);

	return 0;
}

static int changeip_cmd(struct sockaddr *src_addr, socklen_t sin_size, 
	int sendfd, struct enumd_cmd_hd *hd, struct ifreq* ifr)
{
	uint8_t *ip;
	char buf[BUFFER_SIZE];
	struct enumd_ChIp_payload *chip = (struct enumd_ChIp_payload *)hd->payload;

	gethostname(buf, BUFFER_SIZE);
	if(strncmp(buf, chip->devid.hostname, BUFFER_SIZE) || 
		memcmp(chip->devid.mac, ifr->ifr_hwaddr.sa_data, 6))
		return -1;

	ip = (uint8_t*)&(chip->ip);
	snprintf(buf, sizeof(buf), "ifconfig %s %d.%d.%d.%d", 
		ifr->ifr_name, ip[3], ip[2], ip[1], ip[0]);

	system(buf);

	//Change ip reply
	sendto(sendfd, hd, ENUM_TOTAL_SIZE(hd), 0, src_addr, sin_size);

	return 0;
}

static int process_revdata(struct sockaddr *src_addr, socklen_t sin_size, 
	struct enumd_cmd_hd *hd, int size, const char *eth_name)
{
	int sendfd;
	struct ifreq ifr;

	sendfd = socket(AF_INET, SOCK_DGRAM, 0);

	strcpy(ifr.ifr_name, eth_name);
	if (setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)))
		fprintf(stderr, "can't bind to interface %s", ifr.ifr_name);

	if(ioctl(sendfd, SIOCGIFHWADDR, &ifr)<0){	/*获取mac地址*/
		fprintf(stderr, "failed to get mac %s", ifr.ifr_name);
	}

	switch(hd->cmd){
	case list:
		list_cmd(src_addr, sin_size, sendfd, hd, (uint8_t*)ifr.ifr_hwaddr.sa_data);
		break;
	case change_ip:
		changeip_cmd(src_addr, sin_size, sendfd, hd, &ifr);
		break;
	}

	return 0;
}

static int receive_data(struct sockaddr *src_addr, socklen_t sin_size, struct enumd_cmd_hd *hd, int size, const char *eth_name)
{
	if(!check_crc(hd, size))
		return -1;

	return process_revdata(src_addr, sin_size, hd, size, eth_name);
}

int enumd_main(int argc, char** argv)
{
	struct sockaddr_in src_addr; /* 连接对方的地址信息 */ 
	socklen_t sin_size;
	char buf[BUFFER_SIZE];
	int retval=0;
	struct enumd_cmd_hd *hd;
	const char *eth_name="eth0";

	if(argc>=2)
		eth_name = argv[1];

	sin_size = sizeof(src_addr);
	retval = recvfrom(0, buf, sizeof(buf), 0, (struct sockaddr *)&src_addr, &sin_size);
	if (retval){
		hd = (struct enumd_cmd_hd *)buf;
		if(hd->magic != ENUM_MAGIC){	//old interface
			gethostname(buf,sizeof(buf));
			retval = sendto(1, buf, sizeof(buf), 0, (struct sockaddr *)&src_addr, sin_size);
			return 0;
		}

		//new interface
		return receive_data((struct sockaddr *)&src_addr, sin_size, hd, retval, eth_name);
	}
	return 0;
}

