#include <stdio.h>
#include "initroot_startup.h"

static int loop(void) 
{
	for(;;);
}

int main(void)
{
	char **argv;
	int argc;

	startup_begin(&argc, &argv);
	startup_end(loop);
	return 0;
}
