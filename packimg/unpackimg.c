#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include "packimg.h"

static int nand_read(int fd, loff_t offs, uint32_t size, void *buff)
{
	assert(lseek64(fd, offs, SEEK_SET) == offs);
	assert(read(fd, buff, size) == size);
	return 0;
}

uint32_t cal_crc(void* buff, int size)
{
	int i;
	uint32_t *p = (uint32_t *)buff, crc=0;
	
	for (i = 0; i < size; i += 4)
		crc += *p++;

	return crc;
}

//return -1: end of device, 0: ok, 1:try next block
static int dump_pack_once(int fd, loff_t noffs)
{
	struct pack_header phd;
	struct pack_entry *pen;
	int ret=-1;
	uint32_t *p, crc;
	loff_t offset;
	unsigned int i, size;

	if(lseek64(fd, noffs, SEEK_SET) != noffs)
		return -1;

	ret = read(fd, &phd, sizeof(phd));
	if(ret!=sizeof(phd)){
		fprintf(stderr, "end of device when read phd\n");
		return -1;
	}
	
	if(phd.magic != PACK_MAGIC){
		fprintf(stderr, "bad header try next\n");
		return 1;
	}

	size = phd.nentry * sizeof(struct pack_entry);
	pen = malloc(size);
	assert(pen);
	ret = read(fd, pen, size);
	if(ret!=size){
		fprintf(stderr, "end of device when read pack entry\n");
		ret = -1;
		goto end;
	}

	crc = cal_crc(pen, size);
	if(crc != phd.crc){
		fprintf(stderr, "bad crc try next block\n");
		ret = 1;
		goto end;
	}

	for(i=0;i<phd.nentry;i++){
		FILE *pf;

		ret = -1;
		offset = noffs+pen[i].offset;
		if(lseek64(fd, offset, SEEK_SET) != offset){
			fprintf(stderr, "end of device when seek pack%d\n", i);
			goto end;
		}

		size = pen[i].size;
		p = malloc(size);
		if(read(fd, p, size)!=size){
			fprintf(stderr, "end of device when read pack %d\n", i);
			free(p);
			goto end;
		}

		crc = cal_crc(p, size);
		if(crc != pen[i].crc){
			fprintf(stderr, "bad crc@%d try next block\n", i);
			free(p);
			ret = 1;
			goto end;
		}

		pf = fopen(pen[i].name, "w");
		if(!pf){
			fprintf(stderr, "failed to create file %s\n", pen[i].name);
			free(p);
			goto end;
		}
		fwrite(p, size, 1, pf);
		fclose(pf);

		free(p);
		printf("%s@0x%x\n", pen[i].name, pen[i].ldaddr);

	}

	ret = 0;

end:
	free(pen);
	return ret;
}

static int dump_pack(int fd)
{
	mtd_info_t mtd;
	loff_t noffs=0;
	int t;
	
	assert((t = open("/dev/mtd0", O_RDONLY)) >= 0);
	assert(ioctl(t, MEMGETINFO, &mtd) == 0);
	close(t);

	while((t=dump_pack_once(fd,noffs))>0){
		noffs += mtd.erasesize;
	}

	return t;
}

static void print_usage(char *cmd)
{
	printf("Usage: %s [option] [output]\n"
			"\t\t-d <blockrom device>\n", cmd);
}

int unpackimg_main(int argc, char **argv)
{
	const char *dev_name=NULL;
	int dfd, ret;

	while ((ret = getopt(argc, argv, "hd:")) != -1) {
		switch(ret) {
		case 'd':
			dev_name = optarg;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return 0;
		}
	}

	assert(dev_name);
	assert((dfd = open(dev_name, O_RDONLY)) >= 0);

	ret = dump_pack(dfd);

	close(dfd);

	return ret;
}

