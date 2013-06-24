#ifndef COMMAND_H
#define COMMAND_H

typedef struct
{
	char *command;
	void (*fun)(int argc, char *argv[]);
	int (*platform_match)(const char *platform);	//return nonzero if match
}command_t;

void deal_command(char *cmdline);

//usage: BUILDIN_CMD("cmd", cmd_func) or BUILDIN_CMD("cmd", cmd_func, match_func)
#define BUILDIN_CMD(c,f,...)	static command_t f##_buildin_cmd \
	__attribute__((__used__)) __attribute__((section("buildin_cmd"))) = {c, f, ##__VA_ARGS__}

#endif //command.h
