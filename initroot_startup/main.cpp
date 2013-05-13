#include <stdio.h>
#include <dirent.h>
#include <iostream>
#include "initroot_startup.h"

/*
static int ls(void)
{
	DIR *dir_ptr;
    struct dirent *direntp;

    dir_ptr = opendir("/");
    if (dir_ptr == NULL) {
		perror("open root dir fail\n");
		return -1;
    }

	while (1) {
		while ((direntp = readdir(dir_ptr)) != NULL) {
			std::string name(direntp->d_name);
			if (name == "." || name == "..")
				continue;
			if (direntp->d_type == DT_DIR)
				name += '/';
			std::cout << name << " ";
		}
		std::cout << std::endl;
		sleep(1);
		rewinddir(dir_ptr);
	}

    closedir(dir_ptr);
	return 0;
}

static int loop(void) 
{
	return ls();
}
*/

static int ls(void)
{
	DIR *dir_ptr;
    struct dirent *direntp;

    dir_ptr = opendir("/");
    if (dir_ptr == NULL) {
		perror("open root dir fail\n");
		return -1;
    }

	while ((direntp = readdir(dir_ptr)) != NULL) {
		std::string name(direntp->d_name);
		if (name == "." || name == "..")
			continue;
		if (direntp->d_type == DT_DIR)
			name += '/';
		std::cout << name << " ";
	}
	std::cout << std::endl;

    closedir(dir_ptr);
	return 0;
}

static int loop(void) 
{
	int i;
	for (i= 0; i < 10; i++) {
		if (ls())
			return -1;
		sleep(1);
	}
	while (1);
	return 0;
}

int main(void)
{
	char **argv;
	int argc;

	startup_begin(&argc, &argv);
	startup_end(loop);
	return 0;
}
