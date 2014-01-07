#include <stdio.h>
#include <stdint.h>

typedef struct tagBITMAPFILEHEADER {
	unsigned int bfSize;
	unsigned int bfReserved12;
	unsigned int bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
	unsigned int biSize;
	unsigned int biWidth;
	unsigned int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned int biCompression;
	unsigned int biSizeImage;
	unsigned int biXPelsPerMeter;
	unsigned int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
} BITMAPINFOHEADER;

int fillbmp24to16(const char *bmpfile, uint16_t *pbuffer)
{
	int i;
	FILE *pfin;
	char bmp[400];
	int cx, cy;
	BITMAPFILEHEADER bmpfileheader;
	BITMAPINFOHEADER bmpinfoheader;

	uint8_t data[4];
	uint16_t od;
	int ret;

	pfin = fopen(bmpfile, "r");
	if(pfin==NULL){
		printf("file %s open failed\n", bmpfile);
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

	while(fread(data, 3, 1, pfin)){
		od=((data[2]&0xf8)<<8) | ((data[1]&0xfc)<<3) |(data[0]>>3);
		od = (od>>8)|(od<<8);
		*pbuffer++ = od;
	}
	fclose(pfin);

	return 0;	
}

