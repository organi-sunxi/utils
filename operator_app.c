#include <stdio.h>
#include "command.h"

static void run(int argc, char *argv[])
{
	printf("run\n");
}
BUILDIN_CMD("run", run);
	
static void update_app(int argc, char *argv[])
{
	printf("update app\n");
}
BUILDIN_CMD("update-app", update_app);

static void run_startup_app(int argc, char *argv[])
{
	printf("run startup app\n");
}
BUILDIN_CMD("run-startup-app", run_startup_app);

static void update_fastapp(int argc, char *argv[])
{
	printf("update fastapp\n");
}
BUILDIN_CMD("update-fastapp", update_fastapp);

static void remove_fastapp(int argc, char *argv[])
{
	printf("remove fastapp\n");
}
BUILDIN_CMD("remove-fastapp", remove_fastapp);

static void update_fw(int argc, char *argv[])
{
	printf("update fw\n");
}
BUILDIN_CMD("update-fw", update_fw);

