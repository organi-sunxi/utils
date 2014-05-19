#ifndef __ENUM_H__
#define __ENUM_H__

#include <stdint.h>

/**********************************************************\
	hd+payload+crc32(hd+payload)
\**********************************************************/

#define ENUM_MAGIC	0x454e554d

enum enumd_cmd{
	list=0x6c697374,
	change_ip=0x43684970,
};

struct enumd_cmd_hd{
	uint32_t magic;
	enum enumd_cmd cmd;
	uint32_t payload_size;
	char payload[0];
};

struct enumd_devid{
	uint8_t mac[6];
	char hostname[0];
}__attribute__ ((packed));

struct enumd_ChIp_payload{
	uint32_t ip;
	struct enumd_devid devid;
};

#define ENUM_CRC_OFFSET(hd)		(sizeof(struct enumd_cmd_hd)+(hd)->payload_size)
#define ENUM_TOTAL_SIZE(hd)		(ENUM_CRC_OFFSET(hd)+sizeof(int32_t))
#define ENUM_CMD_CRC(hd)	(*(int32_t*)(((char*)(hd))+ENUM_CRC_OFFSET(hd)))

int enumd_main(int argc, char** argv);

#endif
