/*
 * video.c
 *
 *  Copyright (C) 2007, UP-TECH Corporation
 *
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <pthread.h>

#include <linux/fb.h>
#include <linux/videodev2.h>
#include <linux/videodev2.h>

#include "mon-server.h"
#include "video.h"

//#define LOGTIME

#define DEFAULT_FB_DEVICE	"/dev/fb0"
#define VIDEO_CHANNEL_NUM	2

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, x...)	fprintf(logfile, "Debug:"fmt, ## x);
#else
#define DPRINTF(fmt, x...)
#endif

#define LOG(fmt, x...)	if(logfile) fprintf(logfile, fmt, ## x)
static FILE *logfile=NULL;

#define min(a,b)	((a)<(b)?(a):(b))
#define max(a,b)	((a)>(b)?(a):(b))

/* structure used to store information of the buffers */
struct buf_info{
	int index;
	unsigned int length;
	char *start;
};

static unsigned char alaw_mapping_table[256] = {
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
	0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B,
	0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F,
	0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x14,
	0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18, 0x19,
	0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F,
	0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
	0x28, 0x28, 0x29, 0x29, 0x2A, 0x2B, 0x2C, 0x2C, 0x2D, 0x2E, 0x2E, 0x2F, 0x30, 0x30, 0x31, 0x31,
	0x32, 0x33, 0x34, 0x34, 0x35, 0x36, 0x37, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3D, 0x3E,
	0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x5A, 0x5C, 0x5D, 0x5F, 0x60, 0x61, 0x63,
	0x64, 0x66, 0x67, 0x69, 0x6A, 0x6B, 0x6D, 0x6F, 0x70, 0x72, 0x74, 0x75, 0x77, 0x79, 0x7A, 0x7C,
	0x7E, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8A, 0x8B, 0x8D, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9A, 0x9B,
	0x9F, 0xA1, 0xA3, 0xA6, 0xA8, 0xAB, 0xAD, 0xB0, 0xB2, 0xB5, 0xB7, 0xBA, 0xBD, 0xC0, 0xC2, 0xC5,
	0xC8, 0xCB, 0xCE, 0xD1, 0xD4, 0xD6, 0xDA, 0xDD, 0xE0, 0xE3, 0xE6, 0xE9, 0xEC, 0xF0, 0xF4, 0xFA,
};

#define CAPTURE_MAX_BUFFER		3
#define DISPLAY_MAX_BUFFER		3

/* device to be used for capture */
#define CAPTURE_DEVICE		"/dev/video0"
#define CAPTURE_NAME		"Capture"
/* device to be used for display */
#define DISPLAY_DEVICE		"/dev/video1"
#define DISPLAY_NAME		"Display"

#define DEF_PIX_FMT		V4L2_PIX_FMT_UYVY

#define IMG_WIDTH 720
#define IMG_HEIGHT 576

struct display_dev
{
	int fd;	//for frame buffer overlayer device
	unsigned int xpos, ypos, xres, yres;
	unsigned int width, height;	//input buffer
	struct{unsigned int x,y;}pix_step_mod;	//pixel step mod, 1<<pix_step_mod == pre-resize buffer

	struct v4l2_buffer display_buf;
	struct buf_info buff_info[DISPLAY_MAX_BUFFER];
	int numbuffer;
};

static struct display_dev fboverlydev={.fd=-1,};

struct video_dev
{
	int fd;	//for video device
	rect_st crop;
	unsigned long bytesperline;
	int channel;

	struct v4l2_buffer capture_buf;
	struct buf_info buff_info[CAPTURE_MAX_BUFFER];
	int numbuffer;

};
static struct video_dev videodev={.fd=-1, };

static void video_close(struct video_dev *pvdev)
{
	int i;
	struct buf_info *buff_info;

	/* Un-map the buffers */
	for (i = 0; i < CAPTURE_MAX_BUFFER; i++){
		buff_info = &pvdev->buff_info[i];
		if(buff_info->start){
			munmap(buff_info->start, buff_info->length);
			buff_info->start = NULL;
		}
	}

	if(pvdev->fd>=0){
		close(pvdev->fd);
		pvdev->fd = -1;
	}
}

static void display_close(struct display_dev *pfbdev)
{
	int i;
	struct buf_info *buff_info;

	/* Un-map the buffers */
	for (i = 0; i < DISPLAY_MAX_BUFFER; i++){
		buff_info = &pfbdev->buff_info[i];
		if(buff_info->start){
			munmap(buff_info->start, buff_info->length);
			buff_info->start = NULL;
		}
	}

	if (pfbdev->fd >= 0){
		close(pfbdev->fd);
		pfbdev->fd = -1;
	}
}

static int regulate_crop(Prect_st win, Prect_st bound)
{
	unsigned int left, top, right, down;
	
	left = max(win->x, bound->x);
	top = max(win->y, bound->y);
	right = min(win->x+win->width, bound->x+bound->width);
	down = min(win->y+win->height, bound->y+bound->height);

	if(left>=right || top>=down){
		LOG("invalid crop\n");
		return -1;
	}

	win->x = left;
	win->y = top;
	win->width = right-left;
	win->height = down-top;

	return 0;
}


/*===============================initCapture==================================*/
/* This function initializes capture device. It selects an active input
 * and detects the standard on that input. It then allocates buffers in the
 * driver's memory space and mmaps them in the application space.
 */
static int initCapture(const char *dev, struct video_dev *pvdev)
{
	int fd;
	struct v4l2_format fmt;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;

	int ret, i;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	struct v4l2_capability capability;
	struct v4l2_input input;
	v4l2_std_id std_id;
	struct v4l2_standard standard;
	int index;
	Prect_st pvcrop =&pvdev->crop;

	/* Open the capture device */
	fd  = open(dev, O_RDWR);
	if (fd  <= 0) {
		LOG("Cannot open = %s device\n", CAPTURE_DEVICE);
		return -1;
	}
	pvdev->fd = fd;

	/* Get any active input */
	if (ioctl(fd, VIDIOC_G_INPUT, &index) < 0) {
		LOG("VIDIOC_G_INPUT error\n");
		goto ERROR;
	}

	/* Enumerate input to get the name of the input detected */
	memset(&input, 0, sizeof(input));
	input.index = index;
	if (ioctl(fd, VIDIOC_ENUMINPUT, &input) < 0) {
		LOG("VIDIOC_ENUMINPUT error\n");
		goto ERROR;
	}

	LOG("%s: Current Input: %s\n", CAPTURE_NAME, input.name);

	index = pvdev->channel;

	if (ioctl(fd, VIDIOC_S_INPUT, &index) < 0) {
		LOG("VIDIOC_S_INPUT error\n");
		goto ERROR;
	}
	memset(&input, 0, sizeof(input));
	input.index = index;
	if (ioctl(fd, VIDIOC_ENUMINPUT, &input) < 0) {
		LOG("VIDIOC_ENUMINPUT error\n");
		goto ERROR;
	}
	LOG("%s: Input changed to: %s\n", CAPTURE_NAME,
						 input.name);

	/* Store the name of the output as per the input detected */
//	strcpy(inputname, (char*)input.name);

	/* Detect the standard in the input detected */
	pthread_testcancel();
	while(ioctl(fd, VIDIOC_QUERYSTD, &std_id)<0){
		pthread_testcancel();
		LOG("VIDIOC_QUERYSTD error\n");
		sleep(1);
		if (ioctl(fd, VIDIOC_S_INPUT, &index) < 0) {
			LOG("VIDIOC_S_INPUT error\n");
		}
	}
	/* Set the standard*/
	if (ioctl(fd, VIDIOC_S_STD, &std_id) < 0) {
		LOG("VIDIOC_S_STD error\n");
		goto ERROR;
	}

	/* Get the standard*/
	if (ioctl(fd, VIDIOC_G_STD, &std_id) < 0) {
		/* Note when VIDIOC_ENUMSTD always returns EINVAL this
		   is no video device or it falls under the USB exception,
		   and VIDIOC_G_STD returning EINVAL is no error. */
		LOG("VIDIOC_G_STD error\n");
		goto ERROR;
	}
	memset(&standard, 0, sizeof(standard));
	standard.index = 0;
	while (1) {
		if (ioctl(fd, VIDIOC_ENUMSTD, &standard) < 0) {
			LOG("VIDIOC_ENUMSTD error\n");
			goto ERROR;
		}

		/* Store the name of the standard */
		if (standard.id & std_id) {
		//	strcpy(stdname, (char*)standard.name);
			LOG("%s: Current standard: %s\n",
					CAPTURE_NAME, standard.name);
			break;
		}
		standard.index++;
	}

	/* Check if the device is capable of streaming */
	if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
		LOG("VIDIOC_QUERYCAP error\n");
		goto ERROR;
	}

	if (capability.capabilities & V4L2_CAP_STREAMING){
		LOG("%s: Capable of streaming\n", CAPTURE_NAME);
	}
	else {
		LOG("%s: Not capable of streaming\n", CAPTURE_NAME);
		goto ERROR;
	}

	//get default crop size
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_CROPCAP, &cropcap);
	if (ret < 0) {
		LOG("VIDIOC_CROPCAP error\n");
		goto ERROR;
	}

	if(pvcrop->x==0 && pvcrop->y==0 && pvcrop->width==0 && pvcrop->height==0){
		memcpy(pvcrop, &cropcap.defrect, sizeof(*pvcrop));
	}

	ret = regulate_crop(&pvdev->crop, (Prect_st)&cropcap.defrect);
	if(ret < 0)
		goto ERROR;

	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c.left = pvdev->crop.x;
	crop.c.top = pvdev->crop.y;
	crop.c.width = pvdev->crop.width;
	crop.c.height = pvdev->crop.height;
	ret = ioctl(fd, VIDIOC_S_CROP, &crop);
	if (ret < 0) {
		LOG("VIDIOC_S_CROP error, %d %d %d %d\n", crop.c.left, crop.c.top, crop.c.width, crop.c.height);
		goto ERROR;
	}

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		LOG("VIDIOC_G_FMT error\n");
		goto ERROR;
	}

	fmt.fmt.pix.width = pvcrop->width;
	fmt.fmt.pix.height = pvcrop->height;
	fmt.fmt.pix.pixelformat = DEF_PIX_FMT;
	pvdev->bytesperline = fmt.fmt.pix.bytesperline;

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		LOG("VIDIOC_S_FMT error\n");
		goto ERROR;
	}

	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		LOG("VIDIOC_G_FMT error\n");
		goto ERROR;
	}

	if (fmt.fmt.pix.pixelformat != DEF_PIX_FMT) {
		LOG("%s: Requested pixel format not supported\n",
				CAPTURE_NAME);
		goto ERROR;
	}

	LOG("Capture %dx%d\n", pvcrop->width, pvcrop->height);

	/* Buffer allocation
	 * Buffer can be allocated either from capture driver or
	 * user pointer can be used
	 */
	/* Request for MAX_BUFFER input buffers. As far as Physically contiguous
	 * memory is available, driver can allocate as many buffers as
	 * possible. If memory is not available, it returns number of
	 * buffers it has allocated in count member of reqbuf.
	 * HERE count = number of buffer to be allocated.
	 * type = type of device for which buffers are to be allocated.
	 * memory = type of the buffers requested i.e. driver allocated or
	 * user pointer */
	reqbuf.count = CAPTURE_MAX_BUFFER;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		LOG("Cannot allocate memory\n");
		goto ERROR;
	}
	/* Store the number of buffers actually allocated */
	pvdev->numbuffer = reqbuf.count;
	LOG("%s: Number of requested buffers = %d\n", CAPTURE_NAME,
			pvdev->numbuffer);

	memset(&buf, 0, sizeof(buf));

	/* Mmap the buffers
	 * To access driver allocated buffer in application space, they have
	 * to be mmapped in the application space using mmap system call */
	for (i = 0; i < (pvdev->numbuffer); i++) {
		buf.type = reqbuf.type;
		buf.index = i;
		buf.memory = reqbuf.memory;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			LOG("VIDIOC_QUERYCAP error\n");
			pvdev->numbuffer = i;
			goto ERROR;
		}

		pvdev->buff_info[i].length = buf.length;
		pvdev->buff_info[i].index = i;
		pvdev->buff_info[i].start = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				buf.m.offset);

		if (pvdev->buff_info[i].start == MAP_FAILED) {
			LOG("Cannot mmap = %d buffer\n", i);
			pvdev->numbuffer = i;
			goto ERROR;
		}

		memset((void *) pvdev->buff_info[i].start, 0x80,
				pvdev->buff_info[i].length);
		/* Enqueue buffers
		 * Before starting streaming, all the buffers needs to be
		 * en-queued in the driver incoming queue. These buffers will
		 * be used by thedrive for storing captured frames. */
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			LOG("VIDIOC_QBUF error\n");
			pvdev->numbuffer = i + 1;
			goto ERROR;
		}
	}

	LOG("%s: Init done successfully\n\n", CAPTURE_NAME);
	return 0;

ERROR:
	video_close(pvdev);

	return -1;
}

/*
	display output buffer width and height must be 0.5*in <= out < 8*in
	return in>>*step_mode
*/
static int normal_out_step(unsigned int in, unsigned int out, unsigned int *step_mod)
{
	*step_mod=0;
	if(out==0)
		return out;

	while(out<(in>>1)){
		(*step_mod)++;
		in>>=1;
	}

	return in;
}

/*===============================initDisplay==================================*/
/* This function initializes display device. It sets output and standard for
 * LCD. These output and standard are same as those detected in capture device.
 * It, then, allocates buffers in the driver's memory space and mmaps them in
 * the application space */
static int initDisplay(const char *dev, struct display_dev *pddev)
{
	int fd;
	struct v4l2_format fmt;
				
	int ret, i;
	struct v4l2_requestbuffers reqbuf;
	struct v4l2_buffer buf;
	struct v4l2_capability capability;
	struct v4l2_control control;

	/* Open the video display device */
	fd = open(dev, O_RDWR);
	if (fd < 0) {
		LOG("Cannot open = %s device\n", DISPLAY_DEVICE);
		return -1;
	}
	LOG("\n%s: Opened Channel\n", DISPLAY_NAME);
	pddev->fd = fd;

	/* Check if the device is capable of streaming */
	if (ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0) {
		LOG("VIDIOC_QUERYCAP error\n");
		goto ERROR;
	}

	if (capability.capabilities & V4L2_CAP_STREAMING){
		LOG("%s: Capable of streaming\n", DISPLAY_NAME);
	}
	else {
		LOG("%s: Not capable of streaming\n", DISPLAY_NAME);
		goto ERROR;
	}

	/* Rotate by 90 degree so that 480x640 resolution will become 640x480 */
	control.id = V4L2_CID_ROTATE;
	control.value = 0;
	ret = ioctl(fd, VIDIOC_S_CTRL, &control);
	if (ret < 0) {
		LOG("VIDIOC_S_CTRL error\n");
		goto ERROR;
	}

	/* Get the format */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		LOG("VIDIOC_G_FMT error\n");
		goto ERROR;
	}

	pddev->width = fmt.fmt.pix.width = 
		normal_out_step(videodev.crop.width, pddev->xres, &pddev->pix_step_mod.x);

	pddev->height = fmt.fmt.pix.height = 
		normal_out_step(videodev.crop.height, pddev->yres, &pddev->pix_step_mod.y);
	fmt.fmt.pix.pixelformat = DEF_PIX_FMT;

	LOG("Display in buffer=%dx%d, step=%dx%d\n", 
		pddev->width, pddev->height, pddev->pix_step_mod.x, pddev->pix_step_mod.y);

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if (ret < 0) {
		LOG("VIDIOC_S_FMT error\n");
		goto ERROR;
	}

	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if (ret < 0) {
		LOG("VIDIOC_G_FMT error\n");
		goto ERROR;
	}

	if (fmt.fmt.pix.pixelformat != DEF_PIX_FMT) {
		LOG("%s: Requested pixel format not supported\n",
				CAPTURE_NAME);
		goto ERROR;
	}

	/* Get the parameters before setting and
	 * set only required parameters */
	fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
	if(ret<0) {
		LOG("Set Format failed\n");
		goto ERROR;
	}
	fmt.fmt.win.w.left = pddev->xpos;
	fmt.fmt.win.w.top = pddev->ypos;
	fmt.fmt.win.w.width = pddev->xres;
	fmt.fmt.win.w.height = pddev->yres;
	ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
	if(ret<0) {
		LOG("Set Format failed\n");
		goto ERROR;
	}

	/* Buffer allocation
	 * Buffer can be allocated either from capture driver or
	 * user pointer can be used
	 */
	/* Request for MAX_BUFFER input buffers. As far as Physically contiguous
	 * memory is available, driver can allocate as many buffers as
	 * possible. If memory is not available, it returns number of
	 * buffers it has allocated in count member of reqbuf.
	 * HERE count = number of buffer to be allocated.
	 * type = type of device for which buffers are to be allocated.
	 * memory = type of the buffers requested i.e. driver allocated or
	 * user pointer */
	reqbuf.count = DISPLAY_MAX_BUFFER;
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		LOG("Cannot allocate memory\n");
		goto ERROR;
	}
	/* Store the numbfer of buffers allocated */
	
	pddev->numbuffer = reqbuf.count;
	if(pddev->numbuffer>DISPLAY_MAX_BUFFER){
		LOG("%s: number of buffer(%d) over %d\n", DISPLAY_NAME, pddev->numbuffer, DISPLAY_MAX_BUFFER);
		goto ERROR;
	}
		
	LOG("%s: Number of requested buffers = %d\n", DISPLAY_NAME, pddev->numbuffer);

	memset(&buf, 0, sizeof(buf));

	/* Mmap the buffers
	 * To access driver allocated buffer in application space, they have
	 * to be mmapped in the application space using mmap system call */
	for (i = 0; i < pddev->numbuffer; i++) {
		/* Query physical address of the buffers */
		buf.type = reqbuf.type;
		buf.index = i;
		buf.memory = reqbuf.memory;
		ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
		if (ret < 0) {
			LOG("VIDIOC_QUERYCAP error\n");
			(pddev->numbuffer) = i;
			goto ERROR;
		}

		/* Mmap the buffers in application space */
		pddev->buff_info[i].length = buf.length;
		pddev->buff_info[i].index =  i;
		pddev->buff_info[i].start = mmap(NULL, buf.length,
				PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				buf.m.offset);

		if (pddev->buff_info[i].start == MAP_FAILED) {
			LOG("Cannot mmap = %d buffer\n", i);
			pddev->numbuffer = i;
			goto ERROR;
		}
		memset((void *) pddev->buff_info[i].start, 0x80,
				pddev->buff_info[i].length);

		/* Enqueue buffers
		 * Before starting streaming, all the buffers needs to be
		 * en-queued in the driver incoming queue. These buffers will
		 * be used by thedrive for storing captured frames. */
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			LOG("VIDIOC_QBUF error\n");
			pddev->numbuffer = i + 1;
			goto ERROR;
		}
	}

	LOG("%s: Init done successfully\n\n", DISPLAY_NAME);
	return 0;

ERROR:
	display_close(pddev);

	return -1;
}

struct fb_info
{
	int fb_width, fb_height, fb_line_len, fb_size;
	int fb_bpp;
};
static struct fb_info fbinfo;

static int Getfb_info(char *dev)
{
	int fb;
	struct fb_var_screeninfo fb_vinfo;
	struct fb_fix_screeninfo fb_finfo;
	char* fb_dev_name=NULL;
	
	if (!(fb_dev_name = getenv("FRAMEBUFFER")))
		fb_dev_name=dev;

	fb = open (fb_dev_name, O_RDWR);
	if(fb<0){
		DPRINTF("device %s open failed\n", fb_dev_name);
		return -1;
	}
	
	if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_vinfo)) {
		DPRINTF("Can't get VSCREENINFO: %s\n");
		close(fb);
		return -1;
	}

	if (ioctl(fb, FBIOGET_FSCREENINFO, &fb_finfo)) {
		DPRINTF("Can't get FSCREENINFO: %s\n");
		return -1;
	}

	fbinfo.fb_bpp= fb_vinfo.red.length + fb_vinfo.green.length +
		fb_vinfo.blue.length + fb_vinfo.transp.length;

	fbinfo.fb_width = fb_vinfo.xres;
	fbinfo.fb_height = fb_vinfo.yres;
	fbinfo.fb_line_len = fb_finfo.line_length;
	fbinfo.fb_size = fb_finfo.smem_len;

	DPRINTF("frame buffer: %d(%d)x%d,  %dbpp, 0x%xbyte\n", 
		fbinfo.fb_width, fbinfo.fb_line_len, fbinfo.fb_height, fbinfo.fb_bpp, fbinfo.fb_size);
		
/*	if(fbinfo.fb_bpp!=16){
		DPRINTF("frame buffer must be 16bpp mode\n");
		exit(0);
	}*/

	close(fb);

	return 0;
}

static int video_start_cap(struct display_dev *pddev, struct video_dev *pvdev)
{
	int a, ret;

	/* run section
	 * STEP2:
	 * Here display and capture channels are started for streaming. After
	 * this capture device will start capture frames into enqueued
	 * buffers and display device will start displaying buffers from
	 * the qneueued buffers */

	/* Start Streaming. on display device */
	a = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(pddev->fd, VIDIOC_STREAMON, &a);
	if (ret < 0) {
		LOG("display VIDIOC_STREAMON error\n");
		return ret;
	}
	LOG("%s: Stream on...\n", DISPLAY_NAME);

	/* Start Streaming. on capture device */
	a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(pvdev->fd, VIDIOC_STREAMON, &a);
	if (ret < 0) {
		LOG("capture VIDIOC_STREAMON error fd=%d\n", pvdev->fd);
		return ret;
	}
	LOG("%s: Stream on...\n", CAPTURE_NAME);

	/* Set the display buffers for queuing and dqueueing operation */
	pddev->display_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	pddev->display_buf.index = 0;
	pddev->display_buf.memory = V4L2_MEMORY_MMAP;

	/* Set the capture buffers for queuing and dqueueing operation */
	pvdev->capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	pvdev->capture_buf.index = 0;
	pvdev->capture_buf.memory = V4L2_MEMORY_MMAP;

	return 0;
}

static inline int video_cap_frame(struct display_dev *pddev, struct video_dev *pvdev)
{
	int ret;
	int j,h;
	unsigned char *cap_ptr, *dis_ptr, *cap_tmp;
	unsigned int tmp;
#ifdef LOGTIME
	struct timeval btime, ctime, ltime;

	gettimeofday( &btime, NULL );
	ltime = btime;
	LOG("Begin\n");
#endif

	pthread_testcancel();
	/* Dequeue display buffer */
	ret = ioctl(pddev->fd, VIDIOC_DQBUF, &pddev->display_buf);
	if (ret < 0) {
		LOG("Disp VIDIOC_DQBUF");
		return ret;
	}
#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	LOG("DQ Disp: %ld\n", (ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec);
	ltime = ctime;
#endif

	pthread_testcancel();
	/* Dequeue capture buffer */
	ret = ioctl(pvdev->fd, VIDIOC_DQBUF, &pvdev->capture_buf);
	if (ret < 0) {
		LOG("Cap VIDIOC_DQBUF");
		return ret;
	}
#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	LOG("DQ cap: %ld(%ld)\n", 
		(ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec, 
		(ctime.tv_sec-ltime.tv_sec)*1000000 + ctime.tv_usec-ltime.tv_usec);
	ltime = ctime;
#endif

	cap_ptr = (unsigned char*)pvdev->buff_info[pvdev->capture_buf.index].start;
	dis_ptr = (unsigned char*)pddev->buff_info[pddev->display_buf.index].start;

	for (h = 0; h < pddev->height; h++){
		cap_tmp = cap_ptr;
		for(j = 0; j < (pddev->width>>1); j++){
			tmp = cap_tmp[0];	//U
			*dis_ptr++ = alaw_mapping_table[tmp];

			tmp = cap_tmp[1];	//Y
			*dis_ptr++ = alaw_mapping_table[tmp];

			tmp = cap_tmp[2];	//V
			*dis_ptr++ = alaw_mapping_table[tmp];

			tmp = cap_tmp[(1<<(1+pddev->pix_step_mod.x))+1];	//Y
			*dis_ptr++ = alaw_mapping_table[tmp];

			cap_tmp += (1<<(2+pddev->pix_step_mod.x));
		}
		cap_ptr += (pvdev->bytesperline << pddev->pix_step_mod.y);
	}
#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	LOG("Process done: %ld(%ld)\n", 
		(ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec, 
		(ctime.tv_sec-ltime.tv_sec)*1000000 + ctime.tv_usec-ltime.tv_usec);
	ltime = ctime;
#endif

	pthread_testcancel();
	ret = ioctl(pvdev->fd, VIDIOC_QBUF, &pvdev->capture_buf);
	if (ret < 0) {
		LOG("Cap VIDIOC_QBUF");
		return ret;
	}
#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	LOG("Q cap: %ld(%ld)\n", 
		(ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec, 
		(ctime.tv_sec-ltime.tv_sec)*1000000 + ctime.tv_usec-ltime.tv_usec);
	ltime = ctime;
#endif

	pthread_testcancel();
	ret = ioctl(pddev->fd, VIDIOC_QBUF, &pddev->display_buf);
	if (ret < 0) {
		LOG("Disp VIDIOC_QBUF");
		return ret;
	}
	pthread_testcancel();
#ifdef LOGTIME
	gettimeofday( &ctime, NULL );
	tmp = (ctime.tv_sec-btime.tv_sec)*1000000 + ctime.tv_usec-btime.tv_usec;
	LOG("Q disp: %d(%ld), %dfps\n", 
		tmp, (ctime.tv_sec-ltime.tv_sec)*1000000 + ctime.tv_usec-ltime.tv_usec, 1000000/tmp);
#endif
	return 0;
}

static int video_stop_cap(struct display_dev *pddev, struct video_dev *pvdev)
{
	int a, ret;
	
	a = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	ret = ioctl(pddev->fd, VIDIOC_STREAMOFF, &a);
	if (ret < 0) {
		LOG("VIDIOC_STREAMOFF");
		return ret;
	}
	LOG("\n%s: Stream off!!\n", DISPLAY_NAME);

	a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(pvdev->fd, VIDIOC_STREAMOFF, &a);
	if (ret < 0) {
		LOG("VIDIOC_STREAMOFF");
		return ret;
	}

	return 0;
}

struct video_thread{
	pthread_t th_video;
	int video_running;
	pthread_mutex_t mutex;
	int channel;
};

static struct video_thread video_thd;

static void* Show_video(void* v)
{
	if (initCapture(CAPTURE_DEVICE,&videodev) < 0) {
		DPRINTF("Failed to open:%s\n", CAPTURE_DEVICE);
		return NULL;
	}

	if(initDisplay(DISPLAY_DEVICE, &fboverlydev)<0){
		DPRINTF("Failed to open:%s\n", DISPLAY_DEVICE);
		return NULL;
	}

	//start capture
	if( video_start_cap(&fboverlydev, &videodev)<0){
		return NULL;
	}

	while(1){
		pthread_mutex_lock(&video_thd.mutex);
		pthread_mutex_unlock(&video_thd.mutex);

		if(video_cap_frame(&fboverlydev, &videodev)<0)
			break;
	}

	video_stop_cap(&fboverlydev, &videodev);
	return NULL;
}

int video_pause(void)
{
	pthread_mutex_lock(&video_thd.mutex);
	return 0;
}

int video_run(void)
{
	pthread_mutex_unlock(&video_thd.mutex);
	return 0;
}

int video_start(PVideo_st2 pvst)
{
	pthread_mutexattr_t attr;
	Prect_st pshow, pcrop;
	
	if(pvst->channel >= VIDEO_CHANNEL_NUM){
		LOG("valid channel %d\n", pvst->channel);
	}

	pshow = &pvst->show_rect;
	pcrop = &pvst->crop_rect;
	
	if(pshow->x + pshow->width > fbinfo.fb_width ||
		pshow->y + pshow->height > fbinfo.fb_height){
		LOG("Size %dx%d out of screen\n", pshow->x + pshow->width, pshow->y + pshow->height);
		return -1;
	}

	fboverlydev.xpos = pshow->x;
	fboverlydev.ypos = pshow->y;
	fboverlydev.xres = pshow->width;
	fboverlydev.yres = pshow->height;

	videodev.channel = video_thd.channel = pvst->channel;

	memcpy(&videodev.crop, pcrop, sizeof(rect_st));

	if(video_thd.video_running){
		video_stop();
	}

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK_NP);
	pthread_mutex_init(&video_thd.mutex, NULL);

	pthread_create(&video_thd.th_video, NULL, (void*(*)(void*))Show_video, (void*)NULL);
	video_thd.video_running=1;

	return 0;
}

int video_stop(void)
{
	if(video_thd.video_running){
		video_run();
		pthread_cancel(video_thd.th_video);
		pthread_join(video_thd.th_video, NULL);
		video_thd.video_running = 0;
	}

	video_close(&videodev);
	display_close(&fboverlydev);
	pthread_mutex_destroy(&video_thd.mutex);

	return 0;
}

int video_init(FILE *log)
{
	logfile = log;
	video_thd.video_running = 0;
	if(Getfb_info(DEFAULT_FB_DEVICE)<0)
		return -1;

	return 0;
}

