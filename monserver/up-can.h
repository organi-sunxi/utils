#ifndef __UP_CAN_H__
#define __UP_CAN_H__

#ifdef __KERNEL__

#include <linux/time.h>
#include <linux/types.h>
#include <linux/ioctl.h>

#else /* __KERNEL__ */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#endif /* __KERNEL__ */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	BandRate_125kbps=125000,
	BandRate_250kbps=250000,
	BandRate_500kbps=500000,
	BandRate_1Mbps=1000000
}CanBandRate;

#define CAN_MSG_LENGTH	8

typedef struct {
	unsigned int id;	//CAN总线ID
	unsigned char data[CAN_MSG_LENGTH];		//CAN总线数据
	unsigned int dlc;		//数据长度
	unsigned int flags;
}CanData, *PCanData;

#define UPCAN_FLAG_EXCAN					(1<<31)	//extern can flag
#define UPCAN_FLAG_RTR					(1<<30)	//remote frame

/*********************************************************************\
	CAN设备设置接收过滤器结构体
	参数:	IdMask，Mask
			IdFilter，Filter
	是否接收数据按照如下规律:
	Mask	Filter	RevID	Receive
	0		x		x		yes
	1		0		0		yes
	1		0		1		no
	1		1		0		no
	1		1		1		yes
	
\*********************************************************************/
typedef struct{
	unsigned int Mask;
	unsigned int Filter;
	unsigned int flags;	//是否是扩展ID
}CanFilter,*PCanFilter;

/* CAN ioctl magic number */
#define CAN_IOC_MAGIC 'd'

#define UPCAN_IOCTRL_SETBAND	_IOW(CAN_IOC_MAGIC, 0, CanBandRate)	//set can bus band rate
#define UPCAN_IOCTRL_SETLPBK	_IOW(CAN_IOC_MAGIC, 1, unsigned char)	//set can device in loop back mode or normal mode
#define UPCAN_IOCTRL_SETFILTER	_IOW(CAN_IOC_MAGIC, 2, CanFilter)	//set a filter for can device

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif
