#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "lib.h"
#include "command.h"
#include "logmessage.h"

static pid_t app_run_pid=-1;

static int stop_running_app(void)
{
	if(app_run_pid > 0){
		kill(app_run_pid, SIGINT);
		waitpid(app_run_pid, NULL, WNOHANG);
		app_run_pid = -1;

		return 0;
	}

	return 0;
}

static int stop_qtapp()
{
	char cmd[MAX_STRING];
	snprintf(cmd, sizeof(cmd), "killall %s 2>/dev/null", GET_CONF_VALUE(APP_NAME));
	return system(cmd);
}

static int run_app(const char *fullpathname, const char *cd, char *argv[], int system)
{
	int ret;
	pid_t pid;

	stop_running_app();
	
	pid = fork();
	if(pid == 0){//child
		//emconfig exit will stop this app
		prctl(PR_SET_PDEATHSIG, SIGHUP);
		stop_qtapp();
		if(cd && *cd)
			chdir(cd);

		if(!system)
			execv(fullpathname, argv);
		else //search system path
			execvp(fullpathname, argv);
		exit(0);
	}
	else if(pid<0){
		FAILED_OUT("%s", strerror(errno));
		return pid;
	}

	app_run_pid = pid;

	SUCESS_OUT();
	return 0;
}

static int run_qtapp(const char *fullpathname, const char *cd)
{
	int n;
	char *qtargv[MAX_ARGS]={NULL, "-qws"};
	char *env = getenv("QWS_RUN_ARGS");

	parse_args(env, &n, qtargv+2);
	qtargv[n+2]=NULL;

	qtargv[0]=(char*)fullpathname;
	run_app(fullpathname, cd, qtargv, 0);
}

static int run_system_app(const char *cmd, const char *cd)
{
	char *argv[]={NULL, NULL};
	argv[0]=(char*)cmd;
	run_app(cmd, cd, argv, 1);
}

static void run(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'run'");
		return;
	}

	if(run_cmd_quiet(NULL, NULL, "chmod +x %s",	argv[1])<0)
		return;
	
	run_qtapp(argv[1], argc >= 3? argv[2] : NULL);
}
BUILDIN_CMD("run", run);

static void runcmd(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'runcmd'");
		return;
	}

	run_system_app(argv[1], argc >= 3? argv[2] : NULL);
}
BUILDIN_CMD("runcmd", runcmd);

static void stop(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);

	if(stop_running_app()==0)
		SUCESS_OUT();
}
BUILDIN_CMD("stop", stop);

static void update_app(int argc, char *argv[])
{	
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	stop_qtapp();

	if(run_cmd_quiet(NULL, NULL, "chmod +x %s && cp %s %s/%s 2>/dev/null",
				argv[1], argv[1],GET_CONF_VALUE(APP_PATH),GET_CONF_VALUE(APP_NAME)) == 0)
		SUCESS_OUT();
}
BUILDIN_CMD("update-app", update_app);

static void run_startup_app(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);

	run_qtapp(GET_CONF_VALUE(APP_NAME), GET_CONF_VALUE(APP_PATH));
}
BUILDIN_CMD("run-startup-app", run_startup_app);

#define INITFS_IMG_NAME	"initrootfs.img"
#define MKINITFS_PERCENT	70
#define NANDWRITE_PERCENT	30

static void update_fastapp_mkinitfs(void *fg, char out[], int n)
{
	unsigned int p;

	if(sscanf(out, "progress %d\n", &p)<1)
		return;

	p = p*MKINITFS_PERCENT/100;
	PROGRESS_OUT(p, "create fast boot");
}

static void update_fastapp_nandwrite(unsigned long fastappsize, char out[], int n)
{
	unsigned long p;

	if(fastappsize==0)
		return;
	
	sscanf(out, "Writing at 0x%lx\n", &p);

	p = p*NANDWRITE_PERCENT/fastappsize + MKINITFS_PERCENT;
	PROGRESS_OUT(p, "fast boot write");
}

static void update_fastapp(int argc, char *argv[])
{
	const char *mkinitfs=GET_CONF_VALUE(MKINITFS), 
		*tmppath=GET_CONF_VALUE(TMPFILE_PATH),
		*pdev= GET_CONF_VALUE(FASTAPP_DEVICE);
	int ret;
	struct stat fst;

	LOG("%s\n", __FUNCTION__);
	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}

	chdir(tmppath);
	if(run_cmd_quiet(update_fastapp_mkinitfs, NULL, "%s %s "INITFS_IMG_NAME, mkinitfs, argv[1])<0)
		return;
	if( stat(INITFS_IMG_NAME, &fst ) == -1 ){
		FAILED_OUT("get "INITFS_IMG_NAME" size");
		return;
    }
	fst.st_size;
	if(fst.st_size == 0){
		FAILED_OUT("file "INITFS_IMG_NAME" empty");
		return;
	}

	if(run_cmd_quiet(NULL, NULL, "flash_eraseall %s", pdev)<0)
		return;
	if(run_cmd_quiet((out_callback_fun)update_fastapp_nandwrite, (void*)fst.st_size, "nandwrite -p %s "INITFS_IMG_NAME, pdev)<0)
		return;

	unlink(INITFS_IMG_NAME);
	SUCESS_OUT();
}
BUILDIN_CMD("update-fastapp", update_fastapp);

static void remove_fastapp(int argc, char *argv[])
{
	const char *pdev= GET_CONF_VALUE(FASTAPP_DEVICE);
	LOG("%s\n", __FUNCTION__);

	if(run_cmd_quiet(NULL, NULL, "flash_eraseall %s", pdev)==0)
		SUCESS_OUT();
}
BUILDIN_CMD("remove-fastapp", remove_fastapp);

static void update_fw(int argc, char *argv[])
{
	const char *filename, *updateapp=GET_CONF_VALUE(UPDATE_FW_APPNAME),
		*tmppath=GET_CONF_VALUE(TMPFILE_PATH);
	char cmd[MAX_STRING];
	int ret;
	
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments");
		return;
	}
	

	filename = argv[1];

	PROGRESS_OUT(0, "update begin");

	//set PLATFORM env for update script
	setenv("PLATFORM", GET_CONF_VALUE(PLATFORM), 1);

	chdir(tmppath);
	snprintf(cmd, sizeof(cmd), "tar xf %s", filename);
	if(system(cmd)!=0){
		FAILED_OUT("update file decompress error");
		return;
	}
	PROGRESS_OUT(25, "decompress and crc ok");

	snprintf(cmd, sizeof(cmd), "./%s", updateapp);
	if(system(cmd)!=0){
		FAILED_OUT("update error");
		return;
	}

	SUCESS_OUT();
}
BUILDIN_CMD("update-fw", update_fw);

