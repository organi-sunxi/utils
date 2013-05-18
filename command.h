#ifndef COMMAND_H
#define COMMAND_H

typedef struct
{
	char *command;
	void (*fun)(int argc, char *argv[]);
}command_t;

typedef void (*out_callback_fun)(char out[], int n);
int run_cmd_quiet(out_callback_fun fn, const char * format, ...);

void deal_command(char *cmdline);

#define BUILDIN_CMD(c,f)	static command_t f##_buildin_cmd \
	__attribute__((__used__)) __attribute__((section("buildin_cmd"))) = {c, f}

#endif //command.h
