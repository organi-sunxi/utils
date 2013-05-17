#include <stdio.h>
#include "command.h"

static void md5sum(int argc, char *argv[])
{
	printf("md5sum\n");
}
BUILDIN_CMD("md5sum", md5sum);

void update_lib(int argc, char *argv[])
{
	printf("update_lib\n");
}
BUILDIN_CMD("update-lib", update_lib);

