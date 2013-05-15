#include <stdio.h>
#include <string.h>
#include "command.h"
#define MAX_COMMAND 256

int main(int argc, char *argv[])
{
	char cmdline[MAX_COMMAND];

	for (;;) {
		printf(">");
		fgets(cmdline, sizeof(cmdline), stdin);
		deal_command(cmdline);
	}

	return 0;
}
