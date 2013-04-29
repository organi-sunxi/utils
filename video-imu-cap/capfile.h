#ifndef __CAPFILE_H__
#define __CAPFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define VIDEO_MAGIC				0x65646976
#define IMU_MAGIC				0x44554D49

typedef struct {
	uint32_t magic;
	uint16_t width,height;
	uint32_t nframe;	//number of frame
}video_head;

typedef struct {
	struct timeval ts;	//timestamp
	unsigned char data[0];	//payload(YUV), Y(offset:0), UV(offset:wxh)
}video_frame_hd;

typedef struct {
	uint32_t magic;
	uint32_t nframe;	//number of frame
}imu_head;

typedef struct __attribute__((packed)){
	struct timeval ts;	//timestamp
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;

	int16_t accel_x;
	int16_t accel_y;
	int16_t accel_z;

	int16_t mag_x;
	int16_t mag_y;
	int16_t mag_z;
}imu_frame;

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif
