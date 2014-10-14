/************************************************\
*	by threewater<threewater@up-tech.com>	*
*		2007.07.27			*
*						*
\***********************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>

#include <netinet/in.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "mon-server.h"
#include "up-can.h"
#include "video.h"
#include "enum.h"

#define MAX_CAN_DEVICE_NUM	8
#define CAN_DEVICE_NAME_HEAD	"can"
static int can_fd[MAX_CAN_DEVICE_NUM];
static int CAN_DEVICE_NUM=0;

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, x...)	if(logfile) fprintf(logfile, fmt, ## x)
#else
#define DPRINTF(fmt, x...)
#endif

#define LOG(fmt, x...)	if(logfile) fprintf(logfile, fmt, ## x)
static FILE *logfile=NULL;

static pthread_mutex_t send_mutex;

static void ServerSend(void *data, int size)
{
	pthread_mutex_lock(&send_mutex);

	fwrite(data, size, 1, stdout);
	fflush(stdout);

	pthread_mutex_unlock(&send_mutex);
}

static int open_can(const char *name)
{
	int s, ret;
	struct sockaddr_can addr;
	struct ifreq ifr;

	s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(s<0){
		LOG("Create socket failed\n");
		return s;
	}
	
	strcpy(ifr.ifr_name, name);
	ret = ioctl(s, SIOCGIFINDEX, &ifr);
	if(ret<0){
		printf("No device %s\n", name);
		return ret;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	bind(s, (struct sockaddr *)&addr, sizeof(addr));

	return s;
}

static int read_can(int cans, struct can_frame *pframe)
{
	int nbytes;
	pthread_testcancel();
	nbytes = read(cans, pframe, sizeof(struct can_frame));
	pthread_testcancel();
	if (nbytes < 0) {
		LOG("can raw socket read error\n");
		return -1;
	}
	if (nbytes < sizeof(struct can_frame)){
		LOG("read: incomplete CAN frame\n");
		return -1;
	}

	return 0;
}

static int syscan2upcan(struct can_frame *pframe, PCanData pdata)
{
	if(pframe->can_id&CAN_ERR_FLAG)	//error frame
		return -1;

	pdata->dlc = pframe->can_dlc;
	memcpy(pdata->data, pframe->data, pframe->can_dlc);
	pdata->id = pframe->can_id&0x1fffffff;

	pdata->flags = 0;
	if(pframe->can_id&CAN_EFF_FLAG)
		pdata->flags |= UPCAN_FLAG_EXCAN;
	if(pframe->can_id&CAN_RTR_FLAG)
		pdata->flags |= UPCAN_FLAG_RTR;

	return 0;
}

static int upcan2syscan(PCanData pdata, struct can_frame *pframe)
{
	pframe->can_dlc = pdata->dlc;
	memcpy(pframe->data, pdata->data, pframe->can_dlc);
	pframe->can_id = pdata->id&0x1fffffff;

	if(pdata->flags&UPCAN_FLAG_EXCAN)
		pframe->can_id |= CAN_EFF_FLAG;
	if(pdata->flags&UPCAN_FLAG_RTR)
		pframe->can_id |= CAN_RTR_FLAG;

	return 0;
}


static void* canRev(int index)
{
	char package[sizeof(MonServerData)+sizeof(CanData)];
	PCanData pcandata;
	PMonServerData pmsdata;
	struct can_frame frame;
	int fd = can_fd[index];
	int ret;

	if(fd < 0 )
		return NULL;

	pmsdata = (PMonServerData)package;
	pcandata = (PCanData)(pmsdata+1);

	pmsdata->major_cmd = MONCMD_CANBUS;
	pmsdata->minor_cmd0 = index;
	pmsdata->minor_cmd1 = CANBUS_DATA;
	pmsdata->size = sizeof(CanData);

	DPRINTF("can recieve thread begin.\n");
	for(;;){
		ret = read_can(fd, &frame);
		syscan2upcan(&frame,pcandata);
		DPRINTF("\nCan%d id(%c)=0x%x \n", index, (pcandata->flags & UPCAN_FLAG_EXCAN)?'E':'S', pcandata->id);
		if(ret<0)
			break;

		ServerSend(pmsdata, sizeof(package));
	}

	return NULL;
}

static pthread_t th_can[MAX_CAN_DEVICE_NUM];

static int CanBus_Stop(int index)
{
	int fd = can_fd[index];

	if(index >= CAN_DEVICE_NUM || fd<0)
		return -1;

	//cancel thread
	pthread_cancel(th_can[index]);
	/* Wait until all finished */
	pthread_join(th_can[index], NULL);

	close(fd);
	can_fd[index] = -1;

	return 0;
}

static int CanBus_Start(int index, CanBandRate rate)
{
	char str[1024];
	if(index >= CAN_DEVICE_NUM )
		return -1;

	CanBus_Stop(index);

	sprintf(str, "ip link set "CAN_DEVICE_NAME_HEAD"%d down", index);
	system(str);
	sprintf(str, "ip link set "CAN_DEVICE_NAME_HEAD"%d up type can bitrate %d", index, (unsigned int)rate);
	system(str);
	LOG("set bitrate: %s\n", str);

	//if already open, we just chang the bandrate
	if (can_fd[index]<0){
		sprintf(str, CAN_DEVICE_NAME_HEAD"%d", index);
		if((can_fd[index]=open_can(str))<0){
			LOG("Error opening %s can device\n", str);
			return -1;
		}
	}

	/* Create the can receive threads */
	pthread_create(th_can+index, NULL, (void*(*)(void*))canRev, (void*)index);

	return 0;
}

static void CanBus_cmd(PMonServerData pmsdata)
{
	int index = pmsdata->minor_cmd0;
	PCanData pcandata;
	struct can_frame frame;

	if(index >= CAN_DEVICE_NUM)
		return;
	
	pcandata = (PCanData)(pmsdata+1);

	switch(pmsdata->minor_cmd1){
	case CANBUS_DATA:
		if(pmsdata->size != sizeof(CanData) || can_fd[index]<0)
			return;

		upcan2syscan(pcandata, &frame);
		write(can_fd[index], &frame, sizeof(struct can_frame));
		break;

	case CANBUS_START:
		if(pmsdata->size != sizeof(int))
			return;

		CanBus_Start(index, *((int*)(pmsdata+1)));
		break;

	case CANBUS_FILTER:
		if(pmsdata->size != sizeof(CanFilter) || can_fd[index]<0)
			return;
		//ioctl(can_fd[index], UPCAN_IOCTRL_SETFILTER, (pmsdata+1));
		//to do
		break;

	case CANBUS_STOP:
		if(pmsdata->size != 0 || can_fd[index]<0)
			return;
		CanBus_Stop(index);
	}
		
}

static void Video_cmd(PMonServerData pmsdata, int size)
{
	switch(pmsdata->minor_cmd0){
	case VIDEOCMD_START:
		if(size == sizeof(Video_st)){
			Video_st2 vst2;
			PVideo_st pvst = (PVideo_st)(pmsdata+1);
			memcpy(&vst2, pvst, sizeof(Video_st));
			//default set crop_rect = 0
			memset(&vst2.crop_rect, 0, sizeof(rect_st));
			video_start(&vst2);
			LOG("Video start\n");
		}
		else if(size == sizeof(Video_st2)){
			video_start((PVideo_st2)(pmsdata+1));
			LOG("Video start2\n");
		}
		else
			LOG("Wrong video start cmd\n");
		break;
	case VIDEOCMD_PAUSE:
		video_pause();
		LOG("Video pause\n");
		break;
	case VIDEOCMD_RUN:
		video_run();
		LOG("Video run\n");
		break;
	case VIDEOCMD_STOP:
		video_stop();
		LOG("Video stop\n");
		break;
	}
}

static int list_candevice(void)
{
	FILE *netfile;
	char line[1024], *p;

	CAN_DEVICE_NUM = 0;
	netfile = fopen("/proc/net/dev", "r");
	if(netfile==NULL)
		return -1;

	while(fgets(line, sizeof(line), netfile)){
		p = line;
		while(*p==' ' || *p=='\t') p++;
		if(strncmp(p, CAN_DEVICE_NAME_HEAD, strlen(CAN_DEVICE_NAME_HEAD))==0)
			CAN_DEVICE_NUM++;
	}
	
	fclose(netfile);

	LOG("%d can device\n", CAN_DEVICE_NUM);

	return 0;
}

static int monserver_main(int argc, char** argv)
{
	int i, ret;
	static char buffer[1024];
	PMonServerData pmsdata;

	pmsdata = (PMonServerData)buffer;
	DPRINTF("Hello Hello Hello Hello Hello \n");

	pthread_mutex_init (&send_mutex,NULL);
	memset(can_fd, 0xff, sizeof(can_fd));

	if(list_candevice()<0)
		return -1;

	if(video_init(logfile)<0)
		return -1;

	for(;;){
		ret=fread(buffer, 1, sizeof(MonServerData), stdin);
		if(ret<=0)		break;
		
		DPRINTF("get a command(%d),%d %x.%x.%x\n", ret, pmsdata->size, pmsdata->major_cmd, pmsdata->minor_cmd0, pmsdata->minor_cmd1);
		if(pmsdata->size > sizeof(buffer)-ret)
			continue;

		if(pmsdata->size){
			ret=fread(buffer+ret, 1, pmsdata->size, stdin);
			if(ret<=0)		break;
		}
		
		switch(pmsdata->major_cmd){
		case MONCMD_VIDEO:
			Video_cmd(pmsdata, ret);
			break;
		case MONCMD_CANBUS:
			CanBus_cmd(pmsdata);
			break;
		case MONCMD_EXIT:
			goto end;
		}
	}

end:
	video_stop();

	for(i=0; i<CAN_DEVICE_NUM; i++)
		CanBus_Stop(i);

	return 0;
}

//usage: monserver <logfilename>
//		enumd

int main(int argc, char** argv)
{
	int len = strlen(argv[0]), ret;
	char *p = argv[0]+len;

	for(;len>0;len--, p--){
		if(*p == '/'){
			p++;
			break;
		}
	}

	if(argc==2){
		logfile=fopen(argv[1], "w");
		setbuf(logfile, NULL);
	}

	if(strcmp(p, "enumd")==0){
		ret = enumd_main(argc, argv);
	}
	else
		ret = monserver_main(argc, argv);

	if(logfile)
		fclose(logfile);

	return ret;
}
