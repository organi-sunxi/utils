#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib.h"
#include "command.h"
#include "logmessage.h"

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
	int index = -1, argc, i;
	const char *platform = GET_CONF_VALUE(PLATFORM);
	void (*pf)(int argc, char *argv[])=NULL;

	command_t *cmd;

	memset(argv, 0, sizeof(argv));
	parse_args(cmdline, &argc, argv);

	if(argc<=0)
		return;

	for (cmd = __start_buildin_cmd; cmd < __stop_buildin_cmd; cmd++){
		if (strcmp(argv[0], cmd->command)==0){
			if(!cmd->platform_match){
				pf = cmd->fun;
				continue;
			}
			if(cmd->platform_match(platform)){
				cmd->fun(argc, argv);
				return;
			}
		}
	}

	if(pf){	//no match platform use default
		pf(argc, argv);
		return;
	}

	printf("no %s command!\n", argv[0]);
	
}

