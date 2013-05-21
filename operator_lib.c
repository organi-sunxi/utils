#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "command.h"
#include "logmessage.h"

#define MAX_STRING 125

static void md5sum(int argc, char *argv[])
{
	const char *cuslib_path = GET_CONF_VALUE(CUSLIB_PATH);
	char file_path[MAX_STRING] = {'\0'};
	char md5sum_content[MAX_STRING] = {'\0'};
	char md5sum_code[MAX_STRING] = {'\0'};
	FILE *res = NULL;
	int i;

	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'md5sum'");
		return;
	}

	strcpy(file_path, cuslib_path);
	strcat(file_path, "/");
	strcat(file_path, argv[argc - 1]);

	if (access(file_path, 0) < 0) {
		FAILED_OUT(strerror(errno));
		return;
	}
	
	strcpy(md5sum_content, "md5sum ");
	strcat(md5sum_content, file_path);

	res = popen(md5sum_content, "r");
	fread(md5sum_code, sizeof(char), sizeof(md5sum_code), res);

	i = 0;
	while (md5sum_code[i] != ' ') {
		i++;
	}

	md5sum_code[i] = '\0';
	printf("%s\n", md5sum_code);
}
BUILDIN_CMD("md5sum", md5sum);

void update_lib(int argc, char *argv[])
{
	printf("update_lib\n");
}
BUILDIN_CMD("update-lib", update_lib);

