#ifndef __LIB_H__
#define __LIB_H__

enum fb_pol {	// 1 for high valid
	POL_VS		= 1<<0,
	POL_HS 		= 1<<1,
	POL_PCLK	= 1<<2,
	POL_DE		= 1<<3,
	POL_BL		= 1<<4,
	POL_LCDONOFF= 1<<5,
	POL_DEFALT	= (POL_BL|POL_LCDONOFF|POL_DE),

	POL_VALID	= 1<<31,
};

struct fb_info
{
	int width, height;
	int bpp;
	unsigned int pclk;	//in kHz
	unsigned int hfp, hsw, hbp, vfp, vsw, vbp;
	enum fb_pol pol;
};

#define MAX_ARGS 50
#define MAX_STRING 1024

#include "defaultenv.h"

int get_fb_info(struct fb_info *fbinfo);

const char* get_filename(const char* fullpathname, char *path);
unsigned long get_file_size(const char *filename);

int read_device_line(const char *file, char *value, int size);
int write_device_line(const char *file, char *value);
int get_conf_file(const char *conf_file, const char *key, char *value);
int set_conf_file(const char *conf_file, const char *key, const char *value);

#define GET_CONF_VALUE(env)	get_env_default(#env, env)

typedef void (*out_callback_fun)(void *fg, char out[], int n);
const char* get_env_default(const char* env, const char *def);
int run_cmd_quiet(out_callback_fun fn, void *fg, const char * format, ...);

#define FAILED_OUT_HELPER(fmt, ...) printf("failed["fmt"]\n", __VA_ARGS__)
#define FAILED_OUT(...)	FAILED_OUT_HELPER(__VA_ARGS__, 0)
#define SUCESS_OUT()	printf("sucess\n")
#define PROGRESS_OUT_HELPER(p, fmt, ...)	printf("progress %d%%["fmt"]\n", p, __VA_ARGS__)
#define PROGRESS_OUT(p,...)	PROGRESS_OUT_HELPER(p, __VA_ARGS__, 0)

#endif
