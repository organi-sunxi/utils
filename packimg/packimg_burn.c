#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

static void print_usage(char *cmd)
{
	printf("Usage: %s -d /dev/mtd* [-o offset] [-s size] [-c max_copy] -f file\n", cmd);
}

int nand_block_isbad(int fd, loff_t offs)
{
	int ret = ioctl(fd, MEMGETBADBLOCK, &offs);
	assert(ret >= 0);
	return ret;
}

int nand_erase(int fd, loff_t offs, loff_t size)
{
	erase_info_t ei;
	ei.start = offs;
	ei.length = size;
	return ioctl(fd, MEMERASE, &ei);
}

int nand_write(int fd, loff_t offs, uint32_t size, void *buff)
{
	assert(lseek64(fd, offs, SEEK_SET) == offs);
	assert(write(fd, buff, size) == size);
	return 0;
}

int packimg_write(int fd, mtd_info_t *mtd, loff_t nand_off, loff_t nand_size,
				  uint32_t mem_off, uint32_t mem_size, int max_copy)
{
	int err, copies = 0, msize;
	loff_t noffs = nand_off, nsize = nand_size;
	uint32_t moffs;

	if ((nand_off & (mtd->erasesize - 1)) || (nand_size & (mtd->erasesize - 1))) {
		printf("offset %llx and size %llx must be block aligned\n", nand_off, nand_size);
		return -1;
	}

	while (nsize >= mem_size && copies < max_copy) {
		moffs = mem_off;
		msize = mem_size;

		while (msize > 0) {
			uint32_t size;
			while (nand_block_isbad(fd, noffs)) {
				noffs += mtd->erasesize;
				nsize -= mtd->erasesize;
				if (nsize < msize)
					goto out;
			}

			err = nand_erase(fd, noffs, mtd->erasesize);
			if (err) {
				printf("erase block %llx fail\n", noffs);
				goto out;
			}

			size = msize > mtd->erasesize ? mtd->erasesize : msize;
			err = nand_write(fd, noffs, size, (void *)moffs);
			if (err) {
				printf("write block %llx fail\n", noffs);
				goto out;
			}

			moffs += mtd->erasesize;
			msize -= mtd->erasesize;
			noffs += mtd->erasesize;
			nsize -= mtd->erasesize;
		}

		copies++;
	}

out:
	if (copies) {
		printf("success write %d/%d copies of packimg to %llx\n", copies, max_copy, nand_off);
		return 0;
	}
	else {
		printf("fail write packimg to %llx\n", nand_off);
		return -1;
	}
}

#define ROUND(n, size)	((n)+((size)-1))&(~((size)-1))

int packimg_burn_main(int argc, char **argv)
{
	int dfd, ffd;
	int c, max_copy = 1;
	int have_device = 0, have_file = 0;
	char device_name[128], file_name[128];
	loff_t offset = -1, size = -1;
	mtd_info_t mtd_info;
	char *buff;
	struct stat st;

	while ((c = getopt(argc, argv, "hd:o:s:c:f:")) != -1) {
		switch(c) {
		case 'o':
			offset = strtoll(optarg, NULL, 0);
			break;
		case 's':
			size = strtoll(optarg, NULL, 0);
			break;
		case 'c':
			max_copy = strtol(optarg, NULL, 0);
			break;
		case 'd':
			strncpy(device_name, optarg, 128);
			have_device = 1;
			break;
		case 'f':
			strncpy(file_name, optarg, 128);
			have_file = 1;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return 0;
		}
	}

	assert(have_device && have_file);
	assert(max_copy > 0);
	assert((dfd = open(device_name, O_RDWR)) >= 0);
	assert(ioctl(dfd, MEMGETINFO, &mtd_info) == 0);
	assert((ffd = open(file_name, O_RDONLY)) >= 0);

	fstat(ffd, &st);
	assert((buff = malloc(ROUND(st.st_size, mtd_info.writesize))) != NULL);
	assert(read(ffd, buff, st.st_size) == st.st_size);
	memset(buff+st.st_size, 0xff, ROUND(st.st_size, mtd_info.writesize)-st.st_size);

	if (offset < 0)
		offset = 0;
	assert(offset < mtd_info.size);
	if (size < 0)
		size = mtd_info.size - offset;
	assert(size <= mtd_info.size - offset);

	packimg_write(dfd, &mtd_info, offset, size, (uint32_t)buff, ROUND(st.st_size, mtd_info.writesize), max_copy);

	free(buff);
	close(ffd);
	close(dfd);
	return 0;
}
