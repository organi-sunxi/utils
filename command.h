#ifndef COMMAND_H
#define COMMAND_H

typedef struct
{
	char *command;
	void (*fun)(int argc, char *argv[]);
}command_t;

#define CUSLIB                     "CUSLIB_PATH"
#define TMPFILENAME         "TMPFILE_PATH"
#define APPNAME                 "APP_NAME"
#define ETHNAME                 "ETH_NAME"
#define SYSCONF                  "SYS_CONF"
#define HOSTNAMEFILE       "HOSTNAME_FILE"

#define DEFAULT_CUSLIB_PATH            "/data/cuslib"
#define DEFAULT_TMPFILE_PATH          "/tmp"
#define DEFAULT_APP_NAME                 "/data/run/run"
#define DEFAULT_ETH_NAME                 "eth0"
#define DEFAULT_SYS_CONF                  "/data/config"
#define DEFAULT_HOSTNAME_FILE       "/data/hostname"

typedef void (*out_callback_fun)(char out[], int n);
int run_cmd_quiet(out_callback_fun fn, const char * format, ...);

#define FAILED_OUT(...)	printf("failed[%s]\n", ##__VA_ARGS__)
#define SUCESS_OUT()	printf("sucess\n")
#define PROGRESS_OUT(p, ...)	printf("progress %d%%[%s]\n", p, ##__VA_ARGS__)

void deal_command(char *cmdline);

#define BUILDIN_CMD(c,f)	static command_t f##_buildin_cmd \
	__attribute__((__used__)) __attribute__((section("buildin_cmd"))) = {c, f}

#endif //command.h
