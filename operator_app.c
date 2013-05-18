#include <stdio.h>
#include <sys/stat.h>

#include "command.h"
#include "logmessage.h"

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

#define INITFS_IMG_NAME	"initrootfs.img"
#define MKINITFS_PERCENT	70
#define NANDWRITE_PERCENT	30

static unsigned long fastappsize=0;
static void update_fastapp_mkinitfs(char out[], int n)
{
	unsigned int p;

	if(sscanf(out, "progress %d\n", &p)<1)
		return;

	p = p*MKINITFS_PERCENT/100;
	PROGRESS_OUT(p, "create fast boot");
}

static void update_fastapp_nandwrite(char out[], int n)
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
	char *mkinitfs="mkinitfs", *tmppath="/tmp";
	char *pdev="/dev/mtd3";
	int ret;
	struct stat fst;

	LOG("%s", __FUNCTION__);
	if(argc<2)
		return;

	chdir(tmppath);
	if(run_cmd_quiet(update_fastapp_mkinitfs, "%s %s "INITFS_IMG_NAME, mkinitfs, argv[1])<0)
		return;
	if( stat(INITFS_IMG_NAME, &fst ) == -1 ){
		FAILED_OUT("get "INITFS_IMG_NAME" size");
		return;
    }
	fastappsize = fst.st_size;
	if(fst.st_size == 0){
		FAILED_OUT("file "INITFS_IMG_NAME" empty");
		return;
	}

	if(run_cmd_quiet(NULL, "flash_eraseall %s", pdev)<0)
		return;
	if(run_cmd_quiet(update_fastapp_nandwrite, "nandwrite -p %s "INITFS_IMG_NAME, pdev)<0)
		return;

	unlink(INITFS_IMG_NAME);
	SUCESS_OUT();
}
BUILDIN_CMD("update-fastapp", update_fastapp);

static void remove_fastapp(int argc, char *argv[])
{
	char *pdev="/dev/mtd3";
	LOG("%s", __FUNCTION__);

	if(run_cmd_quiet(NULL, "flash_eraseall %s", pdev)==0)
		SUCESS_OUT();
}
BUILDIN_CMD("remove-fastapp", remove_fastapp);

static void update_fw(int argc, char *argv[])
{
	printf("update fw\n");
}
BUILDIN_CMD("update-fw", update_fw);

