#ifndef __IMU_H__
#define __IMU_H__

int imu_stop(void);
void imu_start(void);
int imu_init(FILE *log, FILE *ifile);

#endif
