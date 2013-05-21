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
	char src_file[MAX_STRING] = {'\0'};
	char dest_file[MAX_STRING] = {'\0'};
	char command[MAX_STRING] = {'\0'};
	char cd_command[MAX_STRING] = {'\0'};
	const char *src_path = GET_CONF_VALUE(TMPFILE_PATH);
	const char *dest_path = GET_CONF_VALUE(CUSLIB_PATH);
	int flag = -1;
	
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'update_lib'");
		return;
	}

	strcpy(cd_command, "cd ");
	strcat(cd_command, dest_path);
	strcat(cd_command, "\n");
	
	strcpy(src_file, src_path);
	strcat(src_file, "/");
	strcat(src_file, argv[argc - 1]);
	if (access(src_file, 0) < 0) {
		FAILED_OUT(strerror(errno));
		return;
	}

	memset(command, 0, sizeof(command));
	strcpy(command, cd_command);
	strcat(command, "cp ");
	strcat(command, src_file);
	strcat(command, " .");
	system(command);

	memset(command, 0, sizeof(command));
	strcpy(command, cd_command);
	strcat(command, "ln -s ");
	strcat(command, argv[argc - 1]);
	strcat(command, " ");
	strcat(command, argv[argc - 1]);
	
	memset(dest_file, 0, sizeof(dest_file));
	strcpy(dest_file, dest_path);
	strcat(dest_file, "/");
	strcat(dest_file, argv[argc - 1]);
	flag = 3;
	while (flag--) {
		command[strlen(command) - 2] = '\0';
		dest_file[strlen(dest_file) - 2] = '\0';
		if (access(dest_file, 0) < 0) {
			system(command);
		}
	}	

	SUCESS_OUT();
}
BUILDIN_CMD("update-lib", update_lib);

