#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>

#include <linux/fb.h>
#include <sys/wait.h>

#include "lib.h"
#include "logmessage.h"

int get_fb_info(struct fb_info *fbinfo)
{
	int fb;
	
	struct fb_var_screeninfo fb_vinfo;

	fb = open (GET_CONF_VALUE(FRAMEBUFFER), O_RDWR);
	if(fb<0){
		FAILED_OUT("open fb device");
		return fb;
	}

	if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_vinfo)) {
		FAILED_OUT("Can't get VSCREENINFO\n");
		close(fb);
		return -1;
	}

	fbinfo->bpp= fb_vinfo.red.length + fb_vinfo.green.length +
                fb_vinfo.blue.length + fb_vinfo.transp.length;

	fbinfo->width = fb_vinfo.xres;
	fbinfo->height = fb_vinfo.yres;

	//ps to kHZ
	fbinfo->pclk = 1000000000/fb_vinfo.pixclock;
	fbinfo->hbp = fb_vinfo.left_margin;
	fbinfo->hfp = fb_vinfo.right_margin;
	fbinfo->hsw = fb_vinfo.hsync_len;
	fbinfo->vbp = fb_vinfo.upper_margin;
	fbinfo->vfp = fb_vinfo.lower_margin;
	fbinfo->vsw = fb_vinfo.vsync_len;

	close(fb);
	return 0;
}

const char* get_env_default(const char* env, const char *def)
{
	const char *p = getenv(env);
	if(p)
		return p;

	return def;
}

int read_device_line(const char *file, char *value, int size)
{
	int fd;
	int ret;
	
	fd = open(file, O_RDONLY);
	if (fd<0) {
		FAILED_OUT("%s", strerror(errno));
		return -1;
	}
	ret = read(fd, value, size);
	if(ret<0)
		FAILED_OUT("%s", strerror(errno));

	close(fd);

	return ret;
}

int write_device_line(const char *file, char *value)
{
	int fd;
	int ret;
	
	fd = open(file, O_WRONLY);
	if (fd<0) {
		FAILED_OUT("%s", strerror(errno));
		return -1;
	}
	ret = write(fd, value, strlen(value));
	if(ret<0)
		FAILED_OUT("%s", strerror(errno));
	close(fd);

	return ret;
}

int get_conf_file(const char *conf_file, const char *key, char *value)
{
	char sysconf_line[MAX_STRING], *p;
	int len=strlen(key), ret=0;
	FILE *res = NULL;

	*value = 0;
	res = fopen(conf_file, "r");
	if (!res) {
		FAILED_OUT("%s", strerror(errno));
		return -1;
	}

	while (fgets(sysconf_line, sizeof(sysconf_line), res)) {
		p = sysconf_line+strspn(sysconf_line, " \t");	//remove space or tab
		if (strncmp(p, key, len)==0 && strchr("= \t", p[len])!=NULL) {
			p += len;
			p += strspn(p, " \t");	//remove space or tab
			if(*p!='='){	//can't find value
				ret = 0;
				break;
			}
			p++;
			p += strspn(p, " \t");	//remove space or tab
			for(ret=0; *p!='\n' && *p!=0; ret++)
				*value++ = *p++;
			*value = 0;
			break;
		}
	}

	fclose(res);
	return ret;
}

int set_conf_file(const char *conf_file, const char *key, const char *value)
{
	char sysconf_line[MAX_STRING];
	char *sysconf_content = NULL, *p, *tail;
	long sysconf_size, len=strlen(key);
	FILE *res = NULL;

	LOG("%s\n", __FUNCTION__);

	res = fopen(conf_file, "r");
	if (!res) {
		FAILED_OUT("%s", strerror(errno));
		return -1;
	}

	fseek(res, 0, SEEK_END);
	sysconf_size = ftell(res);
	sysconf_content = (char *)malloc(sysconf_size + MAX_STRING + 1);
	if(!sysconf_content){
		FAILED_OUT("%s", strerror(errno));
		return -1;
	}

	fseek(res, 0, SEEK_SET);
	tail = sysconf_content;
	while (fgets(sysconf_line, sizeof(sysconf_line), res)){
		p = sysconf_line+strspn(sysconf_line, " \t");	//remove space or tab
		if (strncmp(p, key, len)==0 && strchr("= \t", p[len])!=NULL){
			snprintf(sysconf_line, sizeof(sysconf_line), "%s=%s\n", key, value);
		}

		tail+=sprintf(tail, "%s", sysconf_line);
	}
	fclose(res);

	res = fopen(conf_file, "w");
	if (!res) {
		FAILED_OUT("%s", strerror(errno));
		return -1;
	}
	fputs(sysconf_content, res);
	fclose(res);

	free(sysconf_content);

	return 0;
}

int run_cmd_quiet(out_callback_fun fn, void *fg, const char * format,...)
{
	FILE *fp;
	int rc;
	static char cmd[4096];
	va_list arg_ptr;
	va_start(arg_ptr, format);
	vsnprintf(cmd, sizeof(cmd), format, arg_ptr);
	va_end(arg_ptr);

	//printf("##run##%s\n",cmd);

	fp = popen(cmd, "r");
	if(fp == NULL){
		FAILED_OUT("run %s", cmd);
		return -1;
	}

	while(fgets(cmd, sizeof(cmd), fp) != NULL){
		if(fn)
			fn(fg, cmd, sizeof(cmd));
	}

	rc = pclose(fp);
	if(rc == -1){
		FAILED_OUT("close run");
		return -1;
	}

	if(WEXITSTATUS(rc)!=0){
		FAILED_OUT("%s", strerror(errno));
		return -1;
	}
	
	return 0;
}

const char* get_filename(const char* fullpathname, char *path)
{
	const char *p, *pos;
	int n;

	for(pos = p = fullpathname; *p!=0; p++){
		if(*p == '/')
			pos = p+1;
	}

	if(!path)
		return pos;

	n = pos-fullpathname;
	*path = 0;
	if(n==0)
		return pos;

	memcpy(path, fullpathname, n);
	path[n+1]=0;
	
	return pos;
} 

unsigned long get_file_size(const char *filename)
{
	struct stat buf;
	if(stat(filename, &buf)<0){
		FAILED_OUT("%s", strerror(errno));
		return 0;
	}

	return (unsigned long)buf.st_size;
}

