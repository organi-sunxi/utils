#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include "packimg.h"

static void print_usage(char *cmd)
{
	printf("Usage: %s [option] file@loadaddr [file@loadaddr] ... [file@loadaddr] output\n",
		"\t\t-p <pagesize>",
		"\t\t-d <mtd device> default /dev/mtd0", cmd);
}

int packimg_main(int argc, char **argv)
{
	FILE *fout;
	struct pack_header *ph;
	struct pack_entry *pe;
	int header_size, nentry, offset, pagesize=0;
	char **buffer, *end, *dev_name="/dev/mtd0";
	mtd_info_t mtd_info;
	int i;

	while ((i = getopt(argc, argv, "hd:p:")) != -1) {
		switch(i) {
		case 'd':
			dev_name = optarg;
			break;
		case 'p':
			pagesize = strtol(optarg, &end, 0);
			if (end == optarg) {
				fprintf(stderr, "pagesize invalid %s\n", optarg);
				return -1;
			}
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			return 0;
		}
	}

	if (argc < optind+2) {
		print_usage(argv[0]);
		return 0;
	}

	if(pagesize==0){
		//try to get mtd pagesize
		assert((i = open(dev_name, O_RDONLY)) >= 0);
		assert(ioctl(i, MEMGETINFO, &mtd_info) == 0);
		close(i);

		pagesize = mtd_info.writesize;
		printf("pagesize is 0x%x\n", pagesize);
	}

	assert(pagesize!=0);

	nentry = argc - optind - 1;
	header_size = sizeof(struct pack_header) + nentry * sizeof(struct pack_entry);
	offset = (header_size + pagesize - 1) & ~(pagesize - 1);
	ph = malloc(header_size);
	if (ph == NULL) {
		perror("alloc pack header fail\n");
		return -1;
	}
	ph->nentry = nentry;
	pe = (struct pack_entry *)(ph + 1);

	buffer = malloc(nentry * sizeof(char *));
	if (buffer == NULL) {
		perror("alloc entry buffer fail\n");
		return -1;
	}

	for (i = 0; i < nentry; i++) {
		FILE *fin;
		int j;
		char *filename, *loadaddr, *name;
		char *at = strchr(argv[i + optind], '@');
		if (at == NULL) {
			fprintf(stderr, "invalid input %s\n", argv[i + optind]);
			return -1;
		}
		*at = '\0';

		filename = argv[i + optind];
		loadaddr = at + 1;
		fin = fopen(filename, "rb");
		if (fin == NULL) {
			fprintf(stderr, "open file %s fail\n", filename);
			return -1;
		}

		pe[i].ldaddr = strtol(loadaddr, &end, 16);
		if (end == loadaddr) {
			fprintf(stderr, "loadaddr invalid %s\n", loadaddr);
			return -1;
		}

		fseek(fin, 0, SEEK_END);
		pe[i].size = (ftell(fin) + 3) & ~3;
		fseek(fin, 0, SEEK_SET);

		buffer[i] = malloc(pe[i].size);
		if (buffer[i] == NULL) {
			perror("alloc file buffer fail\n");
			return -1;
		}

		fread(buffer[i], pe[i].size, 1, fin);

		pe[i].offset = offset;
		offset += (pe[i].size + pagesize - 1) & ~(pagesize - 1);

		pe[i].crc = 0;
		for (j = 0; j < pe[i].size; j += 4)
			pe[i].crc += *(uint32_t *)(buffer[i] + j);

		name = strrchr(filename, '/');
		if (name == NULL)
			name = filename;
		else
			name++;
		strncpy(pe[i].name, name, PACK_NAME_MAX);

		fclose(fin);
	}

	ph->magic = PACK_MAGIC;
	ph->crc = 0;
	for (i = sizeof(*ph); i < header_size; i += 4)
		ph->crc += *(uint32_t *)((char *)ph + i);

	fout = fopen(argv[argc - 1], "wb");
	if (fout == NULL) {
		fprintf(stderr, "open file %s fail\n", argv[argc - 1]);
		return -1;
	}

	fwrite(ph, header_size, 1, fout);
	for (i = 0; i < nentry; i++) {
		fseek(fout, pe[i].offset, SEEK_SET);
		fwrite(buffer[i], pe[i].size, 1, fout);
		free(buffer[i]);
	}
	fclose(fout);

	free(buffer);
	free(ph);
	
	return 0;
}

extern int packimg_burn_main(int argc, char **argv);
extern int unpackimg_main(int argc, char **argv);

int main(int argc, char **argv)
{
	int len = strlen(argv[0]), ret;
	char *p = argv[0]+len;
	for(;len>0;len--, p--){
		if(*p == '/'){
			p++;
			break;
		}
 	}
 
	if(strcmp(p, "packimg")==0)
		return packimg_main(argc, argv);

	if(strcmp(p, "packimg_burn")==0)
		return packimg_burn_main(argc, argv);

	if(strcmp(p, "unpackimg")==0)
		return unpackimg_main(argc, argv);
}
