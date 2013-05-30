#ifndef COMMAND_H
#define COMMAND_H

#include "defaultenv.h"

#define GET_CONF_VALUE(env)	get_env_default(#env, env)

typedef struct
{
	char *command;
	void (*fun)(int argc, char *argv[]);
}command_t;

#define MAX_STRING 1024

const char* get_filename(const char* fullpathname, char *path);
unsigned long get_file_size(const char *filename);

int read_device_line(const char *file, char *value, int size);
int write_device_line(const char *file, char *value);
int get_conf_file(const char *conf_file, const char *key, char *value);
int set_conf_file(const char *conf_file, const char *key, const char *value);


typedef void (*out_callback_fun)(void *fg, char out[], int n);
const char* get_env_default(const char* env, const char *def);
int run_cmd_quiet(out_callback_fun fn, void *fg, const char * format, ...);

#define FAILED_OUT_HELPER(fmt, ...) printf("failed["fmt"]\n", __VA_ARGS__)
#define FAILED_OUT(...)	FAILED_OUT_HELPER(__VA_ARGS__, 0)
#define SUCESS_OUT()	printf("sucess\n")
#define PROGRESS_OUT_HELPER(p, fmt, ...)	printf("progress %d%%["fmt"]\n", p, __VA_ARGS__)
#define PROGRESS_OUT(p,...)	PROGRESS_OUT_HELPER(p, __VA_ARGS__, 0)

void deal_command(char *cmdline);

#define BUILDIN_CMD(c,f)	static command_t f##_buildin_cmd \
	__attribute__((__used__)) __attribute__((section("buildin_cmd"))) = {c, f}

#endif //command.h