
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define DEFAULT_LOAD_ADDR	0x43100000

#define OUTFILE_NAME	"splash.bin"
#define TMPFILE_NAME	"tmp"
#define SPLASH_MAGIC		0xb87fe731

typedef struct tagBITMAPFILEHEADER {
	uint32_t bfSize;
	uint32_t bfReserved12;
	uint32_t bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
	uint32_t magic;
	uint16_t x, y;
	uint16_t cx, cy;
}splash_header;

static void write_file16(FILE *pfin, FILE *pfout)
{
	unsigned char data[4];
	unsigned short od;

	while(fread(data, 3, 1, pfin)){
		od=((data[2]&0xf8)<<8) | ((data[1]&0xfc)<<3) |(data[0]>>3);
		fwrite(&od, 2, 1, pfout);
	}
}

static void write_file32(FILE *pfin, FILE *pfout)
{
	unsigned char data[4];
	uint32_t od;

	while(fread(data, 3, 1, pfin)){
		memcpy(&od, data, sizeof(data));
		fwrite(&od, sizeof(od), 1, pfout);
	}
}

static void show_help(char *name)
{
	printf("Usage: %s [.bmp file]\n", name);
	printf("\t-x\t x position(default 0)\n\t-y\t y position(default 0)\n");
	printf("\t-b\t LCD bit 16/32(default 16)\n");
	printf("\t-o\t outpout file(default %s)\n", OUTFILE_NAME);
	printf("\t-u\t output uboot image\n");
	printf("\t-a\t uboot image load address(default 0x%x)\n", DEFAULT_LOAD_ADDR);
}

int main(int argc, char* argv[])
{
	int i, bit=16, uimage;
	unsigned long loadaddr=DEFAULT_LOAD_ADDR;
	void (*write_file)(FILE*, FILE*);
	FILE *pfin, *pfout;
	char bmp[400];
	int cx, cy;
	BITMAPFILEHEADER bmpfileheader;
	BITMAPINFOHEADER bmpinfoheader;
	char *outfilename = OUTFILE_NAME;
	splash_header splashhd={SPLASH_MAGIC,};
	int ret;

	if(argc < 2){
		show_help(argv[0]);
		return -1;
	}

	while((i = getopt(argc, argv, "hx:y:b:o:a:u")) != -1)
    {
        switch(i)
        {
            case 'h':
				show_help(argv[0]);
				return -1;
            case 'x':
				splashhd.x = strtol(optarg, NULL, 0);
                break;
            case 'y':
				splashhd.y = strtol(optarg, NULL, 0);
                break;
            case 'b':
				bit = strtol(optarg, NULL, 0);
				if(bit==32)
					write_file = write_file32;
				else if(bit=16)
					write_file = write_file16;
                break;
            case 'o':
				outfilename = optarg;
                break;
			case 'u':
				uimage = 1;
				break;
            case 'a':
				loadaddr = strtol(optarg, NULL, 0);
				break;
        }
    }

	if(!write_file){
		printf("LCD bit only don't support %d\n", bit);
		show_help(argv[0]);
		return -1;
	}

	pfin = fopen(argv[optind], "r");

	if(pfin==NULL){
		printf("file %s open failed\n", argv[optind]);
		return -1;
	}

	fread(bmp, 2, 1, pfin);

	if(bmp[0]!='B' ||bmp[1] !='M'){
		printf("Input file must be bitmap format!\n");
		return -1;
	}

	fread(&bmpfileheader, sizeof(BITMAPFILEHEADER), 1, pfin);
	fread(&bmpinfoheader, sizeof(BITMAPINFOHEADER), 1, pfin);

	cx=bmpinfoheader.biWidth;
	cy=bmpinfoheader.biHeight;

	printf("width: %d, height: %d, %d bit\n", cx, cy, bmpinfoheader.biBitCount);

	if(bmpinfoheader.biBitCount != 24){
		printf("Bitmap file must be 24bit\n");
		return -1;
	}

	pfout = fopen(outfilename, "w");

	if(pfout==NULL){
		printf("Create file %s failed\n", outfilename);
		return -1;
	}

	splashhd.cx = cx;
	splashhd.cy = cy;
	
	fwrite(&splashhd, sizeof(splashhd), 1, pfout);
	fclose(pfout);

	printf("write head ok! Create tmpfile\n");

	pfout = fopen(TMPFILE_NAME, "wb");

	if(pfout==NULL){
		printf("Create file %s failed\n", outfilename);
		return -1;
	}

	write_file(pfin, pfout);

	fclose(pfout);
	fclose(pfin);

	//return 0;
	//system ("ls -l *");

/////////////////////////////////////////////////
	printf("write tmpfile ok! Create gzip...\n");

	ret = system("gzip "TMPFILE_NAME);

	if(ret){
		printf("compress error\n");
		return 1;
	}
	else
		printf("done\n");

	pfout=fopen(outfilename, "a+");
	pfin = fopen(TMPFILE_NAME".gz", "rb");

	while(i=fread(bmp, 1, sizeof(bmp),  pfin)){
		fwrite(bmp, 1, i, pfout);
	}

	fclose(pfout);
	fclose(pfin);
	
	unlink(TMPFILE_NAME".gz");
	rename(outfilename, TMPFILE_NAME);

	if(uimage){
		sprintf(bmp, "mkimage -A arm -C none -a 0x%lx -n splash -d "TMPFILE_NAME" %s", loadaddr, outfilename);
		system(bmp);
		unlink(TMPFILE_NAME);
	}
	return 0;	
}

