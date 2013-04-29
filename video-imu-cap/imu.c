#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <syslog.h>
#include <termios.h>
#include <getopt.h>

#include "imu.h"
#include "capfile.h"


#define BUFSIZE 4096

#define SERIAL_DEV		"/dev/ttyS2"
#define SERIAL_BAUDRATE	B115200

static int serial_fd;
static imu_head imu_hd={IMU_MAGIC,};

FILE *plogfile=NULL, *pimufile=NULL;
#define LOG(fmt, x...)	if(plogfile)	{fprintf(plogfile, fmt, ##x);}

#define ARRAY_SIZE(a)	(sizeof(a)/sizeof(*(a)))

static void* imu_cap_thread(void* v)
{
	char serial_buffer[BUFSIZE];
	int ret;
	imu_frame frame;

	if(!pimufile)
		return NULL;

	imu_hd.nframe = 0;

	for(;;){
		pthread_testcancel();
		ret = read (serial_fd, serial_buffer, BUFSIZE);
		if(ret<=0){
			return NULL;
		}

		ret = sscanf(serial_buffer, "%d,%d,%d;%d,%d,%d;%d,%d,%d", 
			&frame.gyro_x, &frame.gyro_y, &frame.gyro_z,
			&frame.accel_x, &frame.accel_y, &frame.accel_z,
			&frame.mag_x, &frame.mag_y, &frame.mag_z);

		if(ret!=9)	//bad frame
			continue;

		gettimeofday(&frame.ts, NULL);
		imu_hd.nframe++;
		fwrite(&frame, sizeof(frame), 1, pimufile);
	}

	return NULL;
}

static struct termios oldtio,newtio;
static int open_serial(char *serial_dev, speed_t rate)
{
	if((serial_fd = open(serial_dev, O_RDWR))<0){
		fprintf(stderr, "Can't open device %s\n", serial_dev);
		return -1;
	}

  	tcgetattr(serial_fd,&oldtio); /* save current modem settings */
	newtio.c_cflag = CS8 | CLOCAL | CREAD;/*ctrol flag*/
	newtio.c_iflag = IGNPAR; /*input flag*/
	newtio.c_oflag = 0;		/*output flag*/
 	newtio.c_lflag = ICANON;	//enable line input
 	newtio.c_cc[VMIN]=1;
	newtio.c_cc[VTIME]=0;

	cfsetispeed(&newtio, rate);
	cfsetospeed(&newtio, rate);

	/* now clean the modem line and activate the settings for modem */
	tcflush(serial_fd, TCIFLUSH);
	tcflush(serial_fd, TCOFLUSH);
	tcsetattr(serial_fd, TCSANOW, &newtio);/*set attrib */

	return 0;
}

static void close_serial()
{
	tcsetattr(serial_fd,TCSANOW,&oldtio); /* restore old modem setings */
	close(serial_fd);
}

static pthread_t th_imu;

int imu_stop(void)
{
	pthread_cancel(th_imu);
	pthread_join(th_imu, NULL);

	close_serial();

	fseek(pimufile, 0, SEEK_SET);
	fwrite(&imu_hd, sizeof(imu_hd), 1, pimufile);
	return 0;
}

void imu_start(void)
{
	pthread_create(&th_imu, NULL, (void*(*)(void*))imu_cap_thread, (void*)NULL);
}

int imu_init(FILE *log, FILE *ifile)
{
	if(ifile==NULL){
		printf("imu file must be opened\n");
		return -1;
	}

	//skip head, write it when closed
	fseek(ifile, sizeof(imu_hd), SEEK_SET);

	plogfile = log;
	pimufile = ifile;
	
	if(open_serial(SERIAL_DEV, SERIAL_BAUDRATE)<0)
		return -1;

	return 0;
}

