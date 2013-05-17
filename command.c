#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "command.h"

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
	printf("version 1.0.12\n");
}
BUILDIN_CMD("version", version);

static void reboot(int argc, char *argv[])
{
	printf("reboot\n");
}
BUILDIN_CMD("reboot", reboot);

static void exit_cmd(int argc, char *argv[])
{
	printf("exit\n");
	exit(0);
}
BUILDIN_CMD("exit", exit_cmd);


#define STATE_WHITESPACE (0)
#define STATE_WORD (1)
static void parse_args(char *cmdline, int *argc, char **argv)
{
	char *ch = cmdline;
	int state = STATE_WHITESPACE;
	int i = 0;

	*argc = 0;

	while (*ch != '\0') {
		if (state == 0) {
			if (*ch != ' ') {
				argv[i] = ch;
				i++;
				state = STATE_WORD;
			}
		}
		else {
			if ((*ch == ' ') || (*ch == '\n')) {
				*ch = '\0';
				state = STATE_WHITESPACE;
			}
		}

		ch++;
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
	
	for (cmd = __start_buildin_cmd; cmd < __stop_buildin_cmd; cmd++)
	{
		if (!strcmp(argv[0], cmd->command)) {
			cmd->fun(argc, argv);
			return;
		}
	}

	printf("no %s command!\n", argv[0]);
	
}

