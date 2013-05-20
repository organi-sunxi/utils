#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "command.h"
#include "logmessage.h"

#define MAX_COMMAND 256
#define MAX_STRING 256
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

static int flit_string(char *src, char *value_name, char *value_path)
{
	char *tmp = NULL;
	
	if ((!strlen(src)) || (src[0] == '#')) {
		return -1;
	}

	tmp = strchr(src, '=');
	if (!tmp) {
		return -2;
	}

	*tmp++ = '\0';
	strcpy(value_name, src);
	strcpy(value_path, tmp);
	return 0;
}

static void strtrim(char *pstr)
{
	char *src = pstr;

	while (*src != '\0') {
		if ((*src == ' ') || (*src == '\n')) {
			strcpy(src, src + 1);
			continue;
		}

		src++;
	}
}

static void set_env(const char *filename)
{
	char path[MAX_STRING];
	char value_name[MAX_STRING];
	char value_path[MAX_STRING];
	FILE *config_file = fopen(filename, "r");

	if (!config_file) {
		return;
	}
	
	while (fgets(path, sizeof(path), config_file)) {
		strtrim(path);
		if (flit_string(path, value_name, value_path) < 0) {
			continue;
		}

		setenv(value_name, value_path, 1);
	}
	
	fclose(config_file);
}



int main(int argc, char *argv[])
{
	char cmdline[MAX_COMMAND];
	const char *conf_filename=DEFAULT_CONFIG_FILE;

	int next_option;
	const char*const short_options ="hc:l:";

	setvbuf(stdout, NULL, _IONBF, 0);

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
	set_env(conf_filename);

	for (;;) {
		printf(">");
		fgets(cmdline, sizeof(cmdline), stdin);
		deal_command(cmdline);
	}

	if(plogfile)
		fclose(plogfile);

	return 0;
}

