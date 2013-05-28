#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "command.h"
#include "logmessage.h"

static void md5sum_print(void *fg, char out[], int n)
{
	char *ch = out;
	while ((*ch != ' ') && (n--)) {
		ch++;
	}
	*ch = '\0';
	
	printf("%s\n", out);
}

static void md5sum(int argc, char *argv[])
{
	char file_path[MAX_STRING];

	LOG("%s\n", __FUNCTION__);
	
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	snprintf(file_path, sizeof(file_path), "%s/%s", GET_CONF_VALUE(CUSLIB_PATH), argv[1]);

	if (access(file_path, 0) < 0) {
		FAILED_OUT("%s", strerror(errno));
		return;
	}

	run_cmd_quiet(md5sum_print, NULL, "md5sum %s", file_path);
}
BUILDIN_CMD("md5sum", md5sum);

static int update_lib_byname(const char *fullpathname, const char *dest_path)
{
	const char *fullname;
	char name[MAX_STRING], *lp, *postfix=NULL;

	int ret, len;

	if(!dest_path)
		return -1;

	//must be a file
	ret = access(fullpathname, F_OK|R_OK);
	if( ret < 0){
		FAILED_OUT("can't access %s", fullpathname);
		return ret;
	}

	//name must be /path/filename.so or /path/filename.so.x...
	fullname = get_filename(fullpathname);
	strncpy(name, fullname, sizeof(name));

	for(lp = strstr(name, ".so"); lp!=NULL; lp = strstr(lp, ".so")){
		lp +=3;
		postfix = lp;
	}

	if(!postfix){	//no .so in filename
		FAILED_OUT("lib file name %s must include .so", name);
		return -1;
	}

	chdir(dest_path);
	ret = run_cmd_quiet(NULL, NULL, "cp %s .", fullpathname);
	if ( ret < 0)
		return ret;
	len = strlen(postfix);
	if(len==0)
		return 0;
	
	for(lp=postfix+len; postfix<=lp; lp--){
		if(*lp=='.'){
			*lp=0;
			unlink(name);
			ret = run_cmd_quiet(NULL, NULL, "ln -s %s %s", fullname, name);
			if(ret < 0)
				return ret;
		}
	}

	return 0;
}

static void update_lib(int argc, char *argv[])
{
	const char *dest_path = GET_CONF_VALUE(CUSLIB_PATH);
	
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'update_lib'");
		return;
	}

	for(argv++; argc>=2; argv++, argc--){
		if(update_lib_byname(*argv, dest_path)<0)
			return;
	}

	SUCESS_OUT();
}
BUILDIN_CMD("update-lib", update_lib);

