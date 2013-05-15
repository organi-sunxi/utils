#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "command.h"
#include "operator_lib.h"
#include "operator_machine.h"
#include "operator_app.h"

#define MAX_ARGS 50


static void help(int argc, char *argv[]);
static void version(int argc, char *argv[]);
static void reboot(int argc, char *argv[]);

static command_t s_command[] = {
	{"help", help},
	{"version", version},
	{"reboot", reboot},
	{"md5sum", md5sum},
	{"update-lib", update_lib},
	{"get-mac", get_mac},
	{"set-mac", set_mac},
	{"get-ip", get_ip},
	{"set-ip", set_ip},
	{"get-date", get_date},
	{"set-date", set_date},
	{"get-host", get_host},
	{"set-host", set_host},
	{"get-splash", get_splash},
	{"set-splash", set_splash},
	{"get-lcd", get_lcd},
	{"set-lcd", set_lcd},
	{"list-lcd", list_lcd},
	{"get-rotation", get_rotation},
	{"set-rotation", set_rotation},
	{"run", run},
	{"update-app", update_app},
	{"run-startup-app", run_startup_app},
	{"update-fastapp", update_fastapp},
	{"remove-fastapp", remove_fastapp},
	{"update-fw", update_fw}
};

static void help(int argc, char *argv[])
{
	printf("EMconfig Help\n");
}

static void version(int argc, char *argv[])
{
	printf("version 1.0.12\n");
}

static void reboot(int argc, char *argv[])
{
	printf("reboot\n");
}

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

	memset(argv, 0, sizeof(argv));
	parse_args(cmdline, &argc, argv);
	
	for (i = 0; i < sizeof(s_command) / sizeof(command_t); i++)
	{
		if (!strcmp(argv[0], s_command[i].command)) {
			index = i;
		}
	}

	if (index == -1) {
		printf("There is no %s command!\n", argv[0]);
		return;
	}

	s_command[index].fun(argc, argv);
}

