#ifndef COMMAND_H
#define COMMAND_H

typedef struct
{
	char *command;
	void (*fun)(int argc, char *argv[]);
}command_t;

void deal_command(char *cmdline);

#define BUILDIN_CMD(c,f)	static command_t f##_buildin_cmd \
	__attribute__((__used__)) __attribute__((section("buildin_cmd"))) = {c, f}

#endif //command.h
