#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/wait.h>
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

	if(plogfile)
		fclose(plogfile);

	exit(0);
}
BUILDIN_CMD("exit", exit_cmd);

int run_cmd_quiet(out_callback_fun fn, const char * format,...)
{
	FILE *fp;
	int rc;
	char cmd[1024];
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
			fn(cmd, sizeof(cmd));
	}

	rc = pclose(fp);
	if(rc == -1){
		FAILED_OUT("close run");
		return -1;
	}

	if(WEXITSTATUS(rc)!=0){
		FAILED_OUT("exit code %d", WEXITSTATUS(rc));
		return -1;
	}
	
	return 0;
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

