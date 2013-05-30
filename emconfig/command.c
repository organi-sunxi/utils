#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "command.h"
#include "logmessage.h"

#define MAX_ARGS 50

extern command_t __start_buildin_cmd[];
extern command_t __stop_buildin_cmd[];

static void help(int argc, char *argv[])
{
	//list command
	command_t *cmd;
	for (cmd = __start_buildin_cmd; cmd < __stop_buildin_cmd; cmd++){
		printf("%s\n", cmd->command);
	}
}
BUILDIN_CMD("help", help);

static void version(int argc, char *argv[])
{
	printf("version 1.0.0\n");
}
BUILDIN_CMD("version", version);

static void reboot(int argc, char *argv[])
{
	SUCESS_OUT();
	system("reboot");
}
BUILDIN_CMD("reboot", reboot);

static void exit_cmd(int argc, char *argv[])
{
	printf("exit\n");

	if(plogfile)
		fclose(plogfile);

	exit(0);
}
BUILDIN_CMD("exit", exit_cmd);

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


#define STATE_WHITESPACE (0)
#define STATE_WORD (1)
static void parse_args(char *cmdline, int *argc, char **argv)
{
	char *ch = cmdline;
	int state = STATE_WHITESPACE;

	argv[0]=ch;

	for (*argc = 0; *ch != '\0'; ch++) {
		if ((*ch == ' ') || (*ch == '\n')|| (*ch == '\r')) {
			*ch = '\0';
			state = STATE_WHITESPACE;
			continue;
		}

		if (state == STATE_WHITESPACE){
			argv[*argc] = ch;
			(*argc)++;
			state = STATE_WORD;
		}
	}
}

void deal_command(char *cmdline)
{
	char *argv[MAX_ARGS];
	int index = -1;
	int argc;
	int i;

	command_t *cmd;

	memset(argv, 0, sizeof(argv));
	parse_args(cmdline, &argc, argv);

	if(argc<=0)
		return;

	for (cmd = __start_buildin_cmd; cmd < __stop_buildin_cmd; cmd++){
		if (!strcmp(argv[0], cmd->command)) {
			cmd->fun(argc, argv);
			return;
		}
	}

	printf("no %s command!\n", argv[0]);
	
}

