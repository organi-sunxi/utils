#include <stdio.h>

#include "command.h"
#include "logmessage.h"

static int match_em6000(const char *platform)
{
	return strncmp(platform, "EM6", 3)==0;
}

static void em6000_get_lcd(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);
	
	printf("%s\n", __FUNCTION__);
}

BUILDIN_CMD("get-lcd", em6000_get_lcd, match_em6000);

