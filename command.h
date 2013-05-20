#ifndef COMMAND_H
#define COMMAND_H

#include "defaultenv.h"

#define GET_CONF_VALUE(env)	get_env_default(#env, env)

typedef struct
{
	char *command;
	void (*fun)(int argc, char *argv[]);
}command_t;


typedef void (*out_callback_fun)(char out[], int n);
const char* get_env_default(const char* env, const char *def);
int run_cmd_quiet(out_callback_fun fn, const char * format, ...);

#define FAILED_OUT(...)	printf("failed[%s]\n", ##__VA_ARGS__)
#define SUCESS_OUT()	printf("sucess\n")
#define PROGRESS_OUT(p, ...)	printf("progress %d%%[%s]\n", p, ##__VA_ARGS__)

void deal_command(char *cmdline);

#define BUILDIN_CMD(c,f)	static command_t f##_buildin_cmd \
	__attribute__((__used__)) __attribute__((section("buildin_cmd"))) = {c, f}

#endif //command.h
