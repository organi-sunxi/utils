
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sysexits.h>

#define DEFAULT_LOAD_ADDR	0x43100000

#define OUTFILE_NAME	"splash.bin"
#define TMPFILE_NAME	"tmp"
#define SPLASH_MAGIC		0xb87fe731

typedef struct tagBITMAPFILEHEADER {
	uint8_t hd[2];
	uint32_t bfSize;
	uint32_t bfReserved12;
	uint32_t bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER, *PBITMAPFILEHEADER;

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

static int bitmap32to24(unsigned char *data, unsigned char *out)
{
	memcpy(out, data, 3);
	return 3;
}

#define PAD_TAIL(d, n)	(d) |= (((d)&(1<<n)) ? ((1<<n)-1) : 0)

static int bitmap16to24(unsigned char *data, unsigned char *out)
{
	uint16_t *p = (uint16_t *)data;

	out[0] = *p<<3;
	PAD_TAIL(out[0],3);
	out[1] = (*p>>3)&0xfc;
	PAD_TAIL(out[1],2);
	out[2] = (*p>>8)&0xf8;
	PAD_TAIL(out[2],3);

	return 3;
}

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

static int bitmap24to24(unsigned char *data, unsigned char *out)
{
	memcpy(out, data, 3);
	return 3;
}

struct run_fmt{
	int bit;
	int	uimage;
	unsigned long loadaddr;
	int x, y;
	int width,height;
	char *infilename;
	char *outfilename;
	char *tmpfilename;
	int rm_src;
	int (*bitmapto)(unsigned char *, unsigned char *);
};

static int splash2bmp(struct run_fmt *fmt)
{
	splash_header splashhd;
	char buf[1024], data[4], od[4];
	int byteperline, w, h, ret, byteperpixel;
	FILE *pfin, *pfout;
	BITMAPFILEHEADER bmpfileheader;
	BITMAPINFOHEADER bmpinfoheader;

	pfin = fopen(fmt->infilename, "r");
	if(pfin==NULL){
		printf("file %s open failed\n", fmt->infilename);
		return EX_NOINPUT;
	}

	if(fmt->uimage){
		printf("not support uImage file\n");
		return EX_DATAERR;
	}

	fread(&splashhd, sizeof(splashhd), 1, pfin);
	if(splashhd.magic != SPLASH_MAGIC){
		printf("wrong splash file %s\n", fmt->infilename);
		return EX_DATAERR;
	}

	//write splash data to tmp.gz file
	snprintf(buf, sizeof(buf), "%s.gz", fmt->tmpfilename);
	pfout = fopen(buf, "w");
	if(pfout==NULL){
		printf("file %s creat failed\n", buf);
		return EX_CANTCREAT;
	}
	while(ret=fread(buf, 1, sizeof(buf),  pfin)){
		fwrite(buf, 1, ret, pfout);
	}

	fclose(pfin);
	fclose(pfout);

	snprintf(buf, sizeof(buf), "gunzip -f %s.gz", fmt->tmpfilename);
	ret = system(buf);
	if(ret){
		printf("decompress error\n");
		return EX_CANTCREAT;
	}
	else
		printf("decompress done\n");

	//gunzip into tmp file
	pfin = fopen(fmt->tmpfilename, "r");
	if(pfin==NULL){
		printf("file %s open failed\n", fmt->tmpfilename);
		return EX_IOERR;
	}

	pfout = fopen(fmt->outfilename, "w");
	if(pfout==NULL){
		printf("file %s creat failed\n", fmt->outfilename);
		return EX_CANTCREAT;
	}

	//initialize bitmap header
	memset(&bmpfileheader, 0, sizeof(bmpfileheader));
	memset(&bmpinfoheader, 0, sizeof(bmpinfoheader));
	bmpfileheader.hd[0]='B';
	bmpfileheader.hd[1]='M';

	byteperpixel = fmt->bit/8;
	byteperline = splashhd.cx * byteperpixel;

	bmpfileheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpfileheader.bfSize= bmpfileheader.bfOffBits + splashhd.cx*splashhd.cy*3;

	bmpinfoheader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfoheader.biWidth = splashhd.cx;
	bmpinfoheader.biHeight= splashhd.cy;
	bmpinfoheader.biPlanes = 1;
	bmpinfoheader.biBitCount = 24;
	bmpinfoheader.biSizeImage = splashhd.cy*byteperline;

	fwrite(&bmpfileheader, sizeof(bmpfileheader), 1, pfout);
	fwrite(&bmpinfoheader, sizeof(bmpinfoheader), 1, pfout);


	ret = fseek(pfin, (splashhd.cy-1)*byteperline, SEEK_CUR);

	for(h=0;h<splashhd.cy;h++){
		for(w=0;w<splashhd.cx;w++){
			ret = fread(data, byteperpixel, 1, pfin);
			if(ret <= 0){
				printf("failed to read\n");
				fclose(pfout);
				fclose(pfin);
				return EX_DATAERR;
			}
			ret = fmt->bitmapto(data, od);
			fwrite(od, ret, 1, pfout);
		}
		fseek(pfin, -2*byteperline, SEEK_CUR);
	}

	if(fmt->rm_src)
		unlink(fmt->infilename);
	
	fclose(pfin);
	fclose(pfout);
	unlink(fmt->tmpfilename);

	return 0;
}


static int bmp2splash(struct run_fmt *fmt)
{
	FILE *pfin, *pfout;
	char buf[1024], data[4], od[4];
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
		return EX_NOINPUT;
	}

	fread(&bmpfileheader, sizeof(BITMAPFILEHEADER), 1, pfin);
	fread(&bmpinfoheader, sizeof(BITMAPINFOHEADER), 1, pfin);

	if(bmpfileheader.hd[0]!='B' || bmpfileheader.hd[1] !='M'){
		printf("Input file must be bitmap format!\n");
		return EX_DATAERR;
	}

	cx=bmpinfoheader.biWidth;
	cy=bmpinfoheader.biHeight;
	byteperline = cx * bmpinfoheader.biBitCount/8;

	printf("width: %d, height: %d, %d bit\n", cx, cy, bmpinfoheader.biBitCount);

	if(bmpinfoheader.biBitCount != 24){
		printf("Bitmap file must be 24bit\n");
		return EX_DATAERR;
	}

	pfout = fopen(fmt->outfilename, "w");
	if(pfout==NULL){
		printf("Create file %s failed\n", fmt->outfilename);
		return EX_CANTCREAT;
	}

	splashhd.cx = cx;
	splashhd.cy = cy;
	
	fwrite(&splashhd, sizeof(splashhd), 1, pfout);
	fclose(pfout);

	printf("write head ok! Create tmpfile\n");

	pfout = fopen(fmt->tmpfilename, "wb");

	if(pfout==NULL){
		printf("Create file %s failed\n", fmt->tmpfilename);
		return EX_CANTCREAT;
	}

	ret = fseek(pfin, (cy-1)*byteperline, SEEK_CUR);

	for(h=0;h<cy;h++){
		for(w=0;w<cx;w++){
			ret = fread(data, 3, 1, pfin);
			if(ret <= 0){
				printf("failed to read\n");
				fclose(pfout);
				fclose(pfin);
				return EX_DATAERR;
			}
			ret = fmt->bitmapto(data, od);
			fwrite(od, ret, 1, pfout);
		}
		fseek(pfin, -2*byteperline, SEEK_CUR);
	}

	fclose(pfout);
	fclose(pfin);

/////////////////////////////////////////////////
	printf("write tmpfile ok! Create gzip...\n");

	snprintf(buf, sizeof(buf), "gzip -f %s", fmt->tmpfilename);
	ret = system(buf);

	if(ret){
		printf("compress error\n");
		return EX_CANTCREAT;
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

	if(fmt->rm_src)
		unlink(fmt->infilename);

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
	printf("\t-t\t tmporary file name(default %s)\n", TMPFILE_NAME);
	printf("\t-r\t revert mode, splash file to 24bit bitmap file\n");
	printf("\t-d\t delete input file when sucess\n");
}

int main(int argc, char* argv[])
{
	int i, rev=0;
	struct run_fmt fmt={
		.bit=16,
		.uimage=0,
		.loadaddr=DEFAULT_LOAD_ADDR,
		.outfilename = OUTFILE_NAME,
		.tmpfilename = TMPFILE_NAME,
		.x=0,
		.y=0,
		.bitmapto=NULL,
		.rm_src = 0,
	};

	if(argc < 2){
		show_help(argv[0]);
		return EX_NOINPUT;
	}

	while((i = getopt(argc, argv, "hx:y:b:o:a:ut:rd")) != -1){
        switch(i)
        {
            case 'h':
				show_help(argv[0]);
				return 0;
            case 'x':
				fmt.x = strtol(optarg, NULL, 0);
                break;
            case 'y':
				fmt.y = strtol(optarg, NULL, 0);
                break;
            case 'b':
				fmt.bit = strtol(optarg, NULL, 0);
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
			case 'r':
				rev = 1;
				break;
			case 'd':
				fmt.rm_src = 1;
				break;
        }
    }

	if(fmt.bit==32)
		fmt.bitmapto = rev ? bitmap32to24 : bitmap24to32;
	else if(fmt.bit=16)
		fmt.bitmapto = rev ? bitmap16to24 : bitmap24to16;
	else if(fmt.bit=24)
		fmt.bitmapto = rev ? bitmap24to24 : bitmap24to24;

	if(!fmt.bitmapto){
		printf("LCD bit only don't support %d\n", fmt.bit);
		show_help(argv[0]);
		return EX_DATAERR;
	}

	if(argc < optind+1){
		printf("no input file\n");
		show_help(argv[0]);
		return EX_NOINPUT;
	}

	fmt.infilename = argv[optind];
	if(rev)
		return splash2bmp(&fmt);

	return bmp2splash(&fmt);
	
}

