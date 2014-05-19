/************************************************\
*	by threewater<threewater@up-tech.com>	*
*		2007.07.27			*
*						*
\***********************************************/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>

#include "monclient.h"

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, x...)	printf("Debug:"fmt, ## x)
#else
#define DPRINTF(fmt, x...)
#endif

static void* Revceive(void* n)
{
	CanData candata;

#ifdef DEBUG
	CanBus_Revc(&candata);
#else
	int ret;
	int i;

	for(;;){
		pthread_testcancel();
		ret = CanBus_Revc(&candata);
		pthread_testcancel();
		if(ret == -1)
			break;

		printf("\nCan%d, id(%c)=0x%x ", ret, (candata.flags & UPCAN_FLAG_EXCAN)?'E':'S', candata.id);
		for(i=0; i<candata.dlc; i++)
			printf("0x%x, ", candata.data[i]);
		printf("\n");
	}
#endif

	return NULL;
}

static int can_send_cmd(int argc, char *argv[])
{
	CanData candata;
	int len, i, dev;

	if(argc<3){
		return -1;
	}

	sscanf(argv[1], "%d", &dev);

	candata.dlc = len = argc-3;
	sscanf(argv[2], "%d", &candata.id);
	for(i=0; i<len; i++){
		unsigned int d;
		sscanf(argv[3+i], "0x%x", &d);
		candata.data[i] = (unsigned char)d;
	}
	candata.flags = 0;

	CanBus_Send(dev,&candata);

	return 0;
}

static int video_stop_cmd(int argc, char *argv[])
{
	return video_stop();
}

static int video_pause_cmd(int argc, char *argv[])
{
	return video_pause();
}

static int video_run_cmd(int argc, char *argv[])
{
	return video_run();
}

static int video_start_cmd(int argc, char *argv[])
{
	Video_st2 vst;

	if(argc<6){
		return -1;
	}

	sscanf(argv[1], "%d", &vst.channel);
	sscanf(argv[2], "%d", &vst.show_rect.x);
	sscanf(argv[3], "%d", &vst.show_rect.y);
	sscanf(argv[4], "%d", &vst.show_rect.width);
	sscanf(argv[5], "%d", &vst.show_rect.height);

	if(argc<10){
		return video_start((PVideo_st)&vst);
	}

	sscanf(argv[6], "%d", &vst.crop_rect.x);
	sscanf(argv[7], "%d", &vst.crop_rect.y);
	sscanf(argv[8], "%d", &vst.crop_rect.width);
	sscanf(argv[9], "%d", &vst.crop_rect.height);

	return video_start2(&vst);

}

typedef int (*cmd_func_t)(int, char *[]);

typedef struct {
	cmd_func_t func;
	char *pCommand;	//命令行模式下的命令，如果为NULL，则不出现在命令行中
	char *pHelp;			//命令行模式下的帮助
}cmd_function;

static int help_callback(int argc, char *argv[]);

static cmd_function help_func={	help_callback, "help", NULL };
static cmd_function video_start_func={video_start_cmd, "vstart", "channel x y width height <crop_x crop_y crop_width crop_height>" };
static cmd_function video_stop_func={video_stop_cmd, "vstop", "" };
static cmd_function video_pause_func={video_pause_cmd, "vpause", "" };
static cmd_function video_run_func={video_run_cmd, "vrun", "" };
static cmd_function can_send_func={can_send_cmd, "send", "dev id 0xd0 0xd1..." };

static cmd_function* sysfunc[]={&help_func, &video_start_func, &video_stop_func,
		&video_pause_func, &video_run_func, &can_send_func};

#define SYSFUNCNUM		(sizeof(sysfunc)/sizeof(*sysfunc))

#define MAX_COMMANDLINE_LENGTH	1024
#define MAX_ARGS (MAX_COMMANDLINE_LENGTH / 4)

static int help_callback(int argc, char *argv[])
{
	int i;
	if(argc<1)
		return -1;

	printf("*-----------------------------------------------------------------------*\n");
	for(i=0;i<SYSFUNCNUM;i++) {
		if(sysfunc[i]->pCommand){
			printf("%s\n",sysfunc[i]->pCommand);
			if(sysfunc[i]->pHelp){
				printf("\tUsage: %s %s\n", sysfunc[i]->pCommand, sysfunc[i]->pHelp);
			}
		}
	}
	printf("*-----------------------------------------------------------------------*\n");
	printf("Type -help follow command for more information.\n");
	return 0;
}


static int GetCommand(char *command, int len)
{
	unsigned char c;
	int i;
	int numRead;
	int maxRead = len - 1;

	for(numRead = 0, i = 0; numRead < maxRead;) {
		/* try to get a byte from the serial port */
		c=getchar();

		if((c == '\r') || (c == '\n')) {
			command[i++] = '\0';
			putchar(c);

			/* print newline */
			return(numRead);
		} else if(c == '\b') { /* FIXME: is this backspace? */
			if(i > 0) {
				i--;
				numRead--;
				/* cursor one position back. */
				printf("\b \b");
			}
		}else {
			command[i++] = c;
			numRead++;

			/* print character */
			putchar(c);
		}
	}
	return(numRead);
}

#define STATE_WHITESPACE (0)
#define STATE_WORD (1)
static void parse_args(char *cmdline, int *argc, char **argv)
{
	char *c;
	int state = STATE_WHITESPACE;
	int i;

	*argc = 0;

	if(strlen(cmdline) == 0)
		return;
	c = cmdline;
	while(*c != '\0') {
		if(*c == '\t')
			*c = ' ';

		c++;
	}
	
	c = cmdline;
	i = 0;

	while(*c != '\0') {
		if(state == STATE_WHITESPACE) {
			if(*c != ' ') {
				argv[i] = c;
				i++;
				state = STATE_WORD;
			}
		} else {
			if(*c == ' ') {
				*c = '\0';
				state = STATE_WHITESPACE;
			}
		}

		c++;
	}
	
	*argc = i;
}

static int get_num_command_matches(char *cmdline)
{
	int i,j;
	int num_matches = 0;
	j=SYSFUNCNUM;	//(sizeof(commands)/sizeof(commandlist_t));
	for(i=0;i<j;i++) {
		if(strcmp(sysfunc[i]->pCommand, cmdline) == 0) 
			num_matches++;
	}

	return num_matches;
}

static int parse_command(char *cmdline)
{
	static char *argv[MAX_ARGS];
	int argc, num_commands;
	int i, ret;

	memset(argv, 0, sizeof(argv));
	parse_args(cmdline, &argc, argv);
	
	if(argc == 0) 
		return 0;
	num_commands = get_num_command_matches(argv[0]);

	if(num_commands < 0)
		return num_commands;
	
	if(num_commands == 0)
		return -1;
	
	if(num_commands > 1)
		return -1;

	for(i=0;i<SYSFUNCNUM;i++) {
		if(strcmp(sysfunc[i]->pCommand, cmdline) == 0) {
			ret = sysfunc[i]->func(argc, argv);
			if(ret < 0)
				printf("\tUsage: %s %s\n", sysfunc[i]->pCommand, sysfunc[i]->pHelp);
			return ret;
		}
	}

	return -1;
}

static void Console_input(void)
{
	static char commandline[MAX_COMMANDLINE_LENGTH];
	int cmmandlen;
	
	printf("Type \"help\" to list all command.\n");
	
	for(;;)
	{
		printf("\n>");
		cmmandlen = GetCommand(commandline,MAX_COMMANDLINE_LENGTH);
		commandline[cmmandlen]=0;
		if(strncmp(commandline, "exit", 4) == 0)
			break;
		if(cmmandlen > 0)
			parse_command(commandline);
	}
}


int main(int argc, char** argv)
{
	int i;
	pthread_t th_rev;

	if(argc < 3){
		printf("Usage: %s <server address> <port>\n", argv[0]);
		return 0;
	}

	sscanf(argv[2], "%d", &i);

	if(Monitor_init(argv[1], i)<0)
		return -1;

	CanBus_Start(0, BandRate_250kbps);
	CanBus_Start(1, BandRate_250kbps);

	/* Create a receive threads */
	pthread_create(&th_rev, NULL, (void*(*)(void*))Revceive, (void*)NULL);

	Console_input();

	CanBus_Stop(0);
	CanBus_Stop(1);

	//cancel all thread
	pthread_cancel(th_rev);

	/* Wait until all finish. */
	pthread_join(th_rev, NULL);


	return Monitor_exit();
}
