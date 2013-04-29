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
#include <unistd.h>
#include <getopt.h>

#include "imu.h"
#include "video.h"

static void print_usage(char* name)
{
	printf("Usage: %s [optional] <capture file>\n", name);
	printf("\t-h \t show this help\n");
	printf("\t-e n\t video capture every n frame\n");
	printf("\t-m n\t Max video file size(MByte)\n");
}

int main(int argc, char** argv)
{
	char vfname[128], ifname[128], *p=vfname;
	FILE *vfile=NULL, *ifile=NULL;
	int c;
	unsigned int e=1, m=0;

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

	if(argc<=1){
		print_usage(argv[0]);
		return 0;
	}


	while(( c = getopt(argc, argv, "he:m:"))!= -1){
		switch(c){
		case 'e':
			e = strtol(optarg, NULL, 0);
			break;
		case 'm':
			m = strtol(optarg, NULL, 0);
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return 0;
		}
	}
 	
	sprintf(vfname, "%sv", argv[optind]);
	sprintf(ifname, "%si", argv[optind]);

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
	video_start(&vst,e,m);
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
