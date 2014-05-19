#ifndef __MON_CLIENT_H__
#define __MON_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "up-can.h"
#include "mon-server.h"

int Monitor_init(char *hostaddr, int port);
int Monitor_exit(void);

int CanBus_Start(int index, CanBandRate rate);
int CanBus_Stop(int index);
int CanBus_Send(int index, PCanData pdata);
int CanBus_SetFilter(int index, PCanFilter pfilter);
int CanBus_Revc(PCanData pdata);

int video_start(PVideo_st pvst);
int video_start2(PVideo_st2 pvst);
int video_stop(void);
int video_pause(void);
int video_run(void);

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif
