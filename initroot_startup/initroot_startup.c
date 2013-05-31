/*
*  Purpose:to replace original init with c init
*          to save time
*  Date:06dec2011
*  mail:adnan.ali@codethink.co.uk
*/

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <linux/fb.h>
#include <linux/sched.h>
#include "initroot_startup.h"

#define FBDEV		"/dev/fb0"
#define FB_DPI		96

static int oldroot;

static void vfsmount()
{
	oldroot = open("/", 0);
//	mount("","/proc","proc",MS_NOSUID,"");
	mount("devtmpfs", "/dev", "devtmpfs", MS_NOSUID, NULL);
	mount(NULL, "/sys", "sysfs", MS_NOSUID, NULL);
	mount(NULL, "/tmp", "ramfs", MS_NOSUID, NULL);
}

static void vfsumount()
{
	if(oldroot==0)
		return;

	unshare(CLONE_FS);

	fchdir(oldroot);
	chroot(".");
	close(oldroot);

	umount2("/sys", MNT_DETACH);
	umount2("/dev", MNT_DETACH);
	umount2("/data", MNT_DETACH);
//	umount2("/", MNT_DETACH);
}

static void end(void)
{
	printf("app exit handler\n");
	vfsumount();
}

static int cal_wh(int *mmW, int *mmH )
{
	struct fb_var_screeninfo fb_var;
	int fb;
	
	fb = open (FBDEV, O_RDWR);
	if(fb<0){
		printf("device %s open failed\n", FBDEV);
		return -1;
	}

	if (ioctl(fb,FBIOGET_VSCREENINFO,&fb_var)<0){
		printf("ioctl FBIOGET_VSCREENINFO\n");
		close(fb);
		return -1;
	}
	close(fb);

	*mmW = fb_var.xres * 254/FB_DPI/10;
	*mmH = fb_var.yres * 254/FB_DPI/10;
	return 0;
}

static void mkInitNodes(void)
{
	const int mode = 0666|S_IFCHR;
	
	mknod("/dev/console", mode, makedev(5,1));
	mknod("/dev/tty", mode, makedev(5,0));
	mknod("/dev/tty0", mode, makedev(4,0));

	mknod("/dev/null", mode, makedev(1,3));
	mknod("/dev/zero", mode, makedev(1,5));
	//mknod("/dev/ttyS0", mode, makedev(4,64));
	//mknod("/dev/ttyS1", mode, makedev(4,65));
	//mknod("/dev/ttyS2", mode, makedev(4,66));
	//mknod("/dev/ttyS3", mode, makedev(4,67));
	mknod("/dev/event0", mode, makedev(13,64));
	mknod("/dev/event1", mode, makedev(13,65));
	mknod("/dev/rtc0", mode, makedev(254,0));
	mknod("/dev/watchdog", mode, makedev(10,130));
	mknod("/dev/fb0", mode, makedev(29,0));
	mknod("/dev/decrypt", mode, makedev(10,63));

	//data
//	mknod("/dev/mtdblock5", 0666|S_IFBLK, makedev(31,5));
}

static void remove_space(char *str, int size)
{
	char *p=str;

	for(;size>0 && *p && *p!='#'; size--){
		if((*p==' ' || *p=='\t') && !isalnum(p[1])){
			p++;
		}
		else{
			*str++ = *p++;
		}
	}
	*str=0;
}

static void SetQtEnv()
{
	int mmW, mmH;
	FILE *pfile;
	char buffer[100];
	const char *rot="";

	pfile = fopen("/etc/config", "r");
	if(pfile){
		while(fgets(buffer, sizeof(buffer), pfile)){
			remove_space(buffer, sizeof(buffer));
			if(strncmp("ROTATE=", buffer, 7)==0){
				rot = buffer+7;
				break;
			}
		}
		fclose(pfile);
	}

	if(cal_wh(&mmW, &mmH)==0){
		sprintf(buffer, "%sLinuxFB:mmWidth=%d:mmHeight=%d", rot, mmW, mmH);
		setenv("QWS_DISPLAY", buffer, 1);
	}

	setenv("QTDIR", "/", 1);
	setenv("QT_QWS_FONTDIR", "/usr/qt4/lib/fonts", 1);
	setenv("FRAMEBUFFER", FBDEV, 1);
	setenv("LANG", "zh_CN", 1);
	setenv("LC_ALL", "zh_CN", 1);
//	setenv("BACK_DOOR", "This library can work without decrypt device", 1);

	//for tslib
	setenv("TSLIB_CALIBFILE", "/etc/pointercal", 1);
	setenv("POINTERCAL_FILE", "/etc/pointercal", 1);
	setenv("TSLIB_CONFFILE", "/etc/ts.conf", 1);
	setenv("TSLIB_TSDEVICE", "/dev/event1", 1);
	setenv("QWS_MOUSE_PROTO", "Tslib:/dev/event1", 1);
}

static char *qt_argv[]={"run","-qws", "-style", "clearlook", NULL};

int startup_begin(int *argc, char ***argv)
{

	*argc = sizeof(qt_argv)/sizeof(qt_argv[0])-1;
	*argv = qt_argv;

	printf("init startup v2.0\n");

	//vfs mount
	vfsmount();
	mkInitNodes();
	
	SetQtEnv();

	return 0;
}

int startup_end(startup_child_func pf)
{
	int ret;

	if (atexit(end) != 0)
		printf("can't register end handler\n");
	
	//tell kernel, i am ok!
	kill(getppid(), SIGUSR1);

	if(!pf)
		return 0;

	printf("qt show\n");
	ret = pf();
	end();

	return ret;
}

