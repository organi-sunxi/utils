#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include "command.h"
#include "logmessage.h"

static int stop_qtapp()
{
	char cmd[MAX_STRING];
	snprintf(cmd, sizeof(cmd), "killall %s", GET_CONF_VALUE(APP_NAME));
	return system(cmd);
}

static pid_t qt_run_pid=-1;
static int stop_running_app(void)
{
	if(qt_run_pid > 0){
		kill(qt_run_pid, SIGINT);
		waitpid(qt_run_pid, NULL, WNOHANG);
		qt_run_pid = -1;
	}
}

static int run_qtapp(const char *fullpathname, const char *cd)
{
	int ret;
	pid_t pid;

	stop_running_app();
	
	pid = fork();
	if(pid == 0){//child
		stop_qtapp();
		if(cd && *cd)
			chdir(cd);

		signal(SIGINT,SIG_DFL);
		execl(fullpathname, fullpathname, "-qws","$QWS_RUN_ARGS", NULL);
		exit(0);
	}
	else if(pid<0){
		FAILED_OUT("%s", strerror(errno));
		return pid;
	}

	qt_run_pid = pid;

	SUCESS_OUT();
	return 0;
}

static void run(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'run'");
		return;
	}

	run_qtapp(argv[1], argc >= 3? argv[2] : NULL);
}
BUILDIN_CMD("run", run);

static void stop(int argc, char *argv[])
{
	LOG("%s\n", __FUNCTION__);

	stop_running_app();
}
BUILDIN_CMD("stop", stop);

static void update_app(int argc, char *argv[])
{	
	LOG("%s\n", __FUNCTION__);

	if (argc < 2) {
		FAILED_OUT("too few arguments to command 'update-app'");
		return;
	}

	stop_qtapp();

	if(run_cmd_quiet(NULL, NULL, "cp %s %s/%s",
				argv[1],GET_CONF_VALUE(APP_PATH),GET_CONF_VALUE(APP_NAME)) == 0)
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
	if(argc<2)
		return;

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
	printf("update fw\n");
}
BUILDIN_CMD("update-fw", update_fw);

