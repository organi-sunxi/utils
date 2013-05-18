#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "command.h"
#include "logmessage.h"

#define MAX_COMMAND 256
#define DEFAULT_CONFIG_FILE	"/data/etc/emconfig.conf"


FILE *plogfile=NULL;

static void print_usage (const char*program_name, int exit_code)
{ 
	printf ("Usage:%s options\n",program_name);
	printf ("-h Display this usage information.\n"
		"-c <configure file>("DEFAULT_CONFIG_FILE")>\n"
		"-l <log_file_name>\n"
		);
	exit (exit_code); 
}

int main(int argc, char *argv[])
{
	char cmdline[MAX_COMMAND];
	const char *conf_filename=DEFAULT_CONFIG_FILE;

	int next_option;
	const char*const short_options ="hc:l:";

	setvbuf(stdout, NULL, _IONBF, MAX_COMMAND);

	do {
		next_option =getopt(argc,argv,short_options); 
		switch (next_option) 
		{ 
		case 'h':
			print_usage (argv[0],0);
			break;
		case 'c':
			conf_filename = optarg;
			break;
		case 'l':
			plogfile = fopen(optarg, "a");
			LOG("open log file\n");
			break; 
		case -1:/*Done with options.*/ 
			break; 
		default:/*Something else:unexpected.*/ 
			print_usage (argv[0],1); 
		}
	}while (next_option !=-1); 

	//to do: set configure file into envirment

	for (;;) {
		printf(">");
		fgets(cmdline, sizeof(cmdline), stdin);
		deal_command(cmdline);
	}

	if(plogfile)
		fclose(plogfile);

	return 0;
}

