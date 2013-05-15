#ifndef COMMAND_H
#define COMMAND_H

typedef struct
{
	char *command;
	void (*fun)(int argc, char *argv[]);
}command_t;

void deal_command(char *cmdline);

#endif //command.h
