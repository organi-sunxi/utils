
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
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

static int bitmap24to16(unsigned char *data, unsigned char *out)
{
	uint16_t *p = (uint16_t *)out;

	*p=((data[2]&0xf8)<<8) | ((data[1]&0xfc)<<3) |(data[0]>>3);
	return 2;
}

static int bitmap24to32(unsigned char *data, unsigned char *out)
{
	memcpy(out, data, 4);
	return 4;
}

struct output_fmt{
	int bit;
	int	uimage;
	unsigned long loadaddr;
	int x, y;
	int width,height;
	char *infilename;
	char *outfilename;
	char *tmpfilename;
	int (*bitmap24to)(unsigned char *, unsigned char *);
};

static int bmp2splash(struct output_fmt *fmt)
{
	FILE *pfin, *pfout;
	char buf[400], data[4], od[4];
	int cx, cy, byteperline, w, h;
	BITMAPFILEHEADER bmpfileheader;
	BITMAPINFOHEADER bmpinfoheader;
	splash_header splashhd={SPLASH_MAGIC,};
	int ret;

	splashhd.x = fmt->x;
	splashhd.y = fmt->y;

	pfin = fopen(fmt->infilename, "r");

	if(pfin==NULL){
		printf("file %s open failed\n", fmt->infilename);
		return -1;
	}

	fread(buf, 2, 1, pfin);

	if(buf[0]!='B' ||buf[1] !='M'){
		printf("Input file must be bitmap format!\n");
		return -1;
	}

	fread(&bmpfileheader, sizeof(BITMAPFILEHEADER), 1, pfin);
	fread(&bmpinfoheader, sizeof(BITMAPINFOHEADER), 1, pfin);

	cx=bmpinfoheader.biWidth;
	cy=bmpinfoheader.biHeight;
	byteperline = cx * bmpinfoheader.biBitCount/8;

	printf("width: %d, height: %d, %d bit\n", cx, cy, bmpinfoheader.biBitCount);

	if(bmpinfoheader.biBitCount != 24){
		printf("Bitmap file must be 24bit\n");
		return -1;
	}

	pfout = fopen(fmt->outfilename, "w");
	if(pfout==NULL){
		printf("Create file %s failed\n", fmt->outfilename);
		return -1;
	}

	splashhd.cx = cx;
	splashhd.cy = cy;
	
	fwrite(&splashhd, sizeof(splashhd), 1, pfout);
	fclose(pfout);

	printf("write head ok! Create tmpfile\n");

	pfout = fopen(fmt->tmpfilename, "wb");

	if(pfout==NULL){
		printf("Create file %s failed\n", fmt->tmpfilename);
		return -1;
	}

	ret = fseek(pfin, (cy-1)*byteperline, SEEK_CUR);

	for(h=0;h<cy;h++){
		for(w=0;w<cx;w++){
			ret = fread(data, 3, 1, pfin);
			if(ret <= 0){
				printf("failed to read\n");
				goto failed1;
			}
			ret = fmt->bitmap24to(data, od);
			fwrite(od, ret, 1, pfout);
		}
		fseek(pfin, -2*byteperline, SEEK_CUR);
	}

failed1:
	fclose(pfout);
	fclose(pfin);

/////////////////////////////////////////////////
	printf("write tmpfile ok! Create gzip...\n");

	snprintf(buf, sizeof(buf), "gzip -f %s", fmt->tmpfilename);
	ret = system(buf);

	if(ret){
		printf("compress error\n");
		return 1;
	}
	else
		printf("done\n");

	pfout=fopen(fmt->outfilename, "a+");
	snprintf(buf, sizeof(buf), "%s.gz", fmt->tmpfilename);
	pfin = fopen(buf, "rb");

	while(ret=fread(buf, 1, sizeof(buf),  pfin)){
		fwrite(buf, 1, ret, pfout);
	}

	fclose(pfout);
	fclose(pfin);
	
	snprintf(buf, sizeof(buf), "%s.gz", fmt->tmpfilename);
	unlink(buf);

	if(fmt->uimage){
		rename(fmt->outfilename, fmt->tmpfilename);
		snprintf(buf, sizeof(buf), "mkimage -A arm -C none -a 0x%lx -n splash -d %s u%s", fmt->loadaddr, fmt->tmpfilename, fmt->outfilename);
		system(buf);
		unlink(fmt->tmpfilename);
	}
	return 0;	
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
	int i;
	struct output_fmt fmt={
		.bit=16,
		.uimage=0,
		.loadaddr=DEFAULT_LOAD_ADDR,
		.outfilename = OUTFILE_NAME,
		.tmpfilename = TMPFILE_NAME,
		.x=0,
		.y=0,
		.bitmap24to=NULL,
	};

	if(argc < 2){
		show_help(argv[0]);
		return -1;
	}

	while((i = getopt(argc, argv, "hx:y:b:o:a:ut:")) != -1){
        switch(i)
        {
            case 'h':
				show_help(argv[0]);
				return -1;
            case 'x':
				fmt.x = strtol(optarg, NULL, 0);
                break;
            case 'y':
				fmt.y = strtol(optarg, NULL, 0);
                break;
            case 'b':
				fmt.bit = strtol(optarg, NULL, 0);
				if(fmt.bit==32)
					fmt.bitmap24to = bitmap24to32;
				else if(fmt.bit=16)
					fmt.bitmap24to = bitmap24to16;
                break;
            case 'o':
				fmt.outfilename = optarg;
                break;
            case 't':
				fmt.tmpfilename = optarg;
                break;
			case 'u':
				fmt.uimage = 1;
				break;
            case 'a':
				fmt.loadaddr = strtol(optarg, NULL, 0);
				break;
        }
    }

	if(!fmt.bitmap24to){
		printf("LCD bit only don't support %d\n", fmt.bit);
		show_help(argv[0]);
		return -1;
	}

	if(argc < optind+1){
		printf("no input file\n");
		show_help(argv[0]);
		return -1;
	}

	fmt.infilename = argv[optind];
	bmp2splash(&fmt);
}

