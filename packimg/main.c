#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PACK_MAGIC 0x4b434150

struct pack_header {
	uint32_t magic;
	uint32_t nentry;
	uint32_t crc;
};

struct pack_entry {
	uint32_t offset;
	uint32_t size;
	uint32_t ldaddr;
	uint32_t crc;
};

void print_usage(char *cmd)
{
	printf("Usage: %s pagesize file@loadaddr [file@loadaddr] ... [file@loadaddr] output", cmd);
}

int main(int argc, char **argv)
{
	FILE *fout;
	struct pack_header *ph;
	struct pack_entry *pe;
	int header_size, nentry, offset, pagesize;
	char **buffer, *end;
	int i;

	if (argc < 4) {
		print_usage(argv[0]);
		return 0;
	}

	pagesize = strtol(argv[1], &end, 16);
	if (end == argv[1]) {
		fprintf(stderr, "pagesize invalid %s\n", argv[1]);
		return -1;
	}

	nentry = argc - 3;
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
		char *filename, *loadaddr;
		char *at = strchr(argv[i + 2], '@');
		if (at == NULL) {
			fprintf(stderr, "invalid input %s\n", argv[i + 2]);
			return -1;
		}
		*at = '\0';

		filename = argv[i + 2];
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
	}
	fclose(fout);

	return 0;
}
