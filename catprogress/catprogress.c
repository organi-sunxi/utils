#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define buffer_size		16*1024
static uint8_t buffer[buffer_size];

static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t n;

	do {
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

static ssize_t safe_write(int fd, const void *buf, size_t count)
{
	ssize_t n;

	do {
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

/*
 * Write all of the supplied buffer out to a file.
 * This does multiple writes as necessary.
 * Returns the amount written, or -1 on an error.
 */
static ssize_t full_write(int fd, const void *buf, size_t len)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (len) {
		cc = safe_write(fd, buf, len);

		if (cc < 0) {
			if (total) {
				/* we already wrote some! */
				/* user can do another write to know the error code */
				return total;
			}
			return cc;	/* write() returns -1 on failure. */
		}

		total += cc;
		buf = ((const char *)buf) + cc;
		len -= cc;
	}

	return total;
}

//return progress 1/10000
static void cat_progress(int fd, ssize_t totalsize)
{
	ssize_t rd;
	off_t total = 0;

	while (1) {

		rd = safe_read(fd, buffer, buffer_size);

		if (!rd) /* eof - all done */
			break;

		if (rd < 0){
			fprintf(stderr, "read error\n");
			break;
		}

		/* dst_fd == -1 is a fake, else... */
		if (full_write(STDOUT_FILENO, buffer, rd) < rd){
			fprintf(stderr, "write error\n");
			break;
		}

		total += rd;
		fprintf(stderr, "progress %2.2f%%\n", (double)total*100/(double)totalsize);
	}
}

static off_t get_file_size(const char *path)	
{
	unsigned long filesize = -1;
	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		return filesize;
	}else{
		filesize = statbuff.st_size;
	}
	return filesize;
}

int main(int argc, char **argv)
{
	int fd;
	off_t size;
	char *filename;

	if(argc==1){
		printf("Read file and write to stdout with progress output on stderr\n");
		printf("Usage: %s <filename>\n", argv[0]);
		return 0;
	}

	filename = argv[1];

	size = get_file_size(filename);
	if(size==-1){
		fprintf(stderr, "Can't get file %s size\n", filename);
		return -1;
	}
	
	fd = open(filename, O_RDONLY);
	if(fd < 0){
		fprintf(stderr, "Can't open file %s\n", filename);
		return -1;
	}

	cat_progress(fd, size);

	close(fd);
	return 0;
}

