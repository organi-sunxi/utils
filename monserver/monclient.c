/************************************************\
*	by threewater<threewater@up-tech.com>	*
*		2007.07.27			*
*						*
\***********************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "monclient.h"

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, x...)	printf("Debug:"fmt, ## x)
#else
#define DPRINTF(fmt, x...)
#endif

static int server_skt=-1;

int Monitor_init(char *hostaddr, int port)
{
	int skt;
	struct sockaddr_in s_in;

	bzero(&s_in,sizeof(s_in));
	s_in.sin_family = AF_INET;
	s_in.sin_addr.s_addr = inet_addr(hostaddr);
	s_in.sin_port = htons(port);

	skt = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(skt, (struct sockaddr *)&s_in, sizeof(struct sockaddr_in)) < 0)
	{
		fprintf(stderr, "Unable to connect to host(%s:%d)\n", hostaddr, port);
		return -1;
	}

	server_skt = skt;

	return 0;
}

int Monitor_exit(void)
{
	if(server_skt<0)
		return -1;
	
	close(server_skt);

	return 0;
}

int CanBus_Start(int index, CanBandRate rate)
{
	char package[sizeof(MonServerData)+sizeof(CanBandRate)];
	CanBandRate *prate;
	PMonServerData pmsdata;

	if(server_skt<0)
		return -1;

	pmsdata = (PMonServerData)package;
	prate = (CanBandRate*)(pmsdata+1);
	
	pmsdata->major_cmd = MONCMD_CANBUS;
	pmsdata->minor_cmd0 = index;
	pmsdata->minor_cmd1 = CANBUS_START;
	pmsdata->size = sizeof(CanBandRate);
	*prate = rate;

	if(send(server_skt, package, sizeof(package), 0)<0)
		return -1;

	return 0;
}


int CanBus_Stop(int index)
{
	char package[sizeof(MonServerData)];
	PMonServerData pmsdata;

	if(server_skt<0)
		return -1;

	pmsdata = (PMonServerData)package;
	
	pmsdata->major_cmd = MONCMD_CANBUS;
	pmsdata->minor_cmd0 = index;
	pmsdata->minor_cmd1 = CANBUS_STOP;
	pmsdata->size = 0;

	if(send(server_skt, package, sizeof(package), 0)<0)
		return -1;

	return 0;
}

int CanBus_Send(int index, PCanData pdata)
{
	char package[sizeof(MonServerData)+sizeof(CanData)];
	PMonServerData pmsdata;

	if(server_skt<0)
		return -1;

	pmsdata = (PMonServerData)package;

	memcpy(pmsdata+1, pdata, sizeof(CanData));
	
	pmsdata->major_cmd = MONCMD_CANBUS;
	pmsdata->minor_cmd0 = index;
	pmsdata->minor_cmd1 = CANBUS_DATA;
	pmsdata->size = sizeof(CanData);

	if(send(server_skt, package, sizeof(package), 0)<0)
		return -1;

	return 0;
}


int CanBus_SetFilter(int index, PCanFilter pfilter)
{
	char package[sizeof(MonServerData)+sizeof(CanFilter)];
	PMonServerData pmsdata;

	if(server_skt<0)
		return -1;

	pmsdata = (PMonServerData)package;

	memcpy(pmsdata+1, pfilter, sizeof(CanFilter));
	
	pmsdata->major_cmd = MONCMD_CANBUS;
	pmsdata->minor_cmd0 = index;
	pmsdata->minor_cmd1 = CANBUS_FILTER;
	pmsdata->size = sizeof(CanFilter);

	if(send(server_skt, package, sizeof(package), 0)<0)
		return -1;

	return 0;
}

//return can channel or -1, -2
int CanBus_Revc(PCanData pdata)
{
#ifdef DEBUG
	char c;
	int ret;
	int i;

	if(server_skt<0)
		return -1;

	DPRINTF("start receiving \n");

	for(;;){
		pthread_testcancel();
		ret = recv(server_skt, &c, 1, 0);
		pthread_testcancel();
		if(ret < 0)
			break;
		putchar(c);
		fflush(stdout);
	}
#else
	MonServerData msdata;
	int ret;

	if(server_skt<0)
		return -1;

	ret = recv(server_skt, &msdata, sizeof(MonServerData), 0);
	if(ret < 0)
		return -1;

	if(msdata.major_cmd != MONCMD_CANBUS && msdata.minor_cmd1 != CANBUS_DATA){
		return -2;
	}

	ret = recv(server_skt, pdata, sizeof(CanData), 0);
	if(ret < 0)
		return -1;

	return msdata.minor_cmd0;
#endif
}

int video_stop(void)
{
	MonServerData msdata;

	if(server_skt<0)
		return -1;

	msdata.major_cmd = MONCMD_VIDEO;
	msdata.minor_cmd0 = VIDEOCMD_STOP;
	msdata.size = 0;

	if(send(server_skt, &msdata, sizeof(msdata), 0)<0)
		return -1;

	return 0;
}


int video_pause(void)
{
	MonServerData msdata;

	if(server_skt<0)
		return -1;

	msdata.major_cmd = MONCMD_VIDEO;
	msdata.minor_cmd0 = VIDEOCMD_PAUSE;
	msdata.size = 0;

	if(send(server_skt, &msdata, sizeof(msdata), 0)<0)
		return -1;

	return 0;
}

int video_run(void)
{
	MonServerData msdata;

	if(server_skt<0)
		return -1;

	msdata.major_cmd = MONCMD_VIDEO;
	msdata.minor_cmd0 = VIDEOCMD_RUN;
	msdata.size = 0;

	if(send(server_skt, &msdata, sizeof(msdata), 0)<0)
		return -1;

	return 0;
}

int video_start(PVideo_st pvst)
{
	char package[sizeof(MonServerData)+sizeof(Video_st)];
	PMonServerData pmsdata;

	if(server_skt<0)
		return -1;

	pmsdata = (PMonServerData)package;
	memcpy(pmsdata+1, pvst, sizeof(Video_st));
	
	pmsdata->major_cmd = MONCMD_VIDEO;
	pmsdata->minor_cmd0 = VIDEOCMD_START;
	pmsdata->size = sizeof(Video_st);

	if(send(server_skt, package, sizeof(package), 0)<0)
		return -1;

	return 0;
}

int video_start2(PVideo_st2 pvst)
{
	char package[sizeof(MonServerData)+sizeof(Video_st2)];
	PMonServerData pmsdata;

	if(server_skt<0)
		return -1;

	pmsdata = (PMonServerData)package;
	memcpy(pmsdata+1, pvst, sizeof(Video_st2));
	
	pmsdata->major_cmd = MONCMD_VIDEO;
	pmsdata->minor_cmd0 = VIDEOCMD_START;
	pmsdata->size = sizeof(Video_st2);

	if(send(server_skt, package, sizeof(package), 0)<0)
		return -1;

	return 0;
}

