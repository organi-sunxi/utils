/*
 * drivers/media/video/sun4i_tvd/test/app_tvd.c
 *
 * (C) Copyright 2007-2012
 * threewater
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>
#include <linux/videodev2.h>
#include <time.h>
#include <linux/fb.h>

#include "imu.h"
#include "video.h"

static void print_usage(char* name)
{
	printf("Usage: %s <capture file>\n", name);
}

int main(int argc, char** argv)
{
	char vfname[128], ifname[128], *p=vfname;
	FILE *vfile=NULL, *ifile=NULL;

	Video_st2 vst={
		.channel = 0,
		.show_rect = {
			.x = 0, 
			.y = 0,
			.width = 640,
			.height = 480,
		},
		.crop_rect = {
			.x = 0,
			.y = 0,
			.width = 720,
			.height = 576,
		},
	};

	if(argc<=1 || strcmp(argv[1],"-h")==0){
		print_usage(argv[0]);
		return 0;
	}
	sprintf(vfname, "%sv", argv[1]);
	sprintf(ifname, "%si", argv[1]);

	vfile = fopen(vfname, "w");
	if(vfile == NULL){
		printf("Can't create file: %s\n", vfname);
		return -1;
	}

	ifile = fopen(ifname, "w");
	if(ifile == NULL){
		printf("Can't create file: %s\n", ifname);
		fclose(vfile);
		return -1;
	}
	
	video_init(stderr, vfile);
	imu_init(stderr, ifile);
	video_start(&vst);
	imu_start();

	printf("press \'stop\' to stop capture\n");

	while(1){
		gets(p);
		if(strcmp(p, "stop")==0)
			break;
	}
	video_stop();
	imu_stop();

	if(vfile)
		fclose(vfile);

	if(ifile)
		fclose(ifile);
	
	return 0;
}
