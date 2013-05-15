#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <FreeImage.h>

void print_usage(char *cmd)
{
	printf("Usage: %s width height filename\n", cmd);
}

unsigned char clamp(double v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return v;
}

int main(int argc, char **argv)
{
	int i, j, n;
	int h=0, w=0;
	unsigned char *byuv, *brgb;
	int syuv, srgb;
	FILE *fyuv, *frgb;
	char nyuv[128], nrgb[128], njpg[128], nbmp[128];

	if (argc != 4) {
		print_usage(argv[0]);
		return 0;
	}

	w = strtol(argv[1], NULL, 0);
	h = strtol(argv[2], NULL, 0);

	snprintf(nyuv, 128, "%s.yuv", argv[3]);
	snprintf(njpg, 128, "%s.jpg", argv[3]);
	snprintf(nbmp, 128, "%s.bmp", argv[3]);

	fyuv = fopen(nyuv, "rb");
	if (fyuv == NULL) {
		printf("can't open file %s\n", nyuv);
		goto err0;
	}

	syuv = w * h * 3 / 2;
	srgb = w * h * 3;

	byuv = malloc(syuv);
	if (byuv == NULL) {
		printf("can't alloc memory\n");
		goto err1;
	}

	brgb = malloc(srgb);
	if (brgb == NULL) {
		printf("can't alloc memory\n");
		goto err2;
	}

	n = fread(byuv, w, h * 3 / 2, fyuv);
	assert(n == h * 3 / 2);

	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
#if 1 // ITU-R YCbCr to RGB
			double y, u, v;
			y = byuv[i * w + j];
			u = byuv[w * h + (i >> 1) * w + (j & ~0x1)] - 128.0;
			v = byuv[w * h + (i >> 1) * w + (j & ~0x1) + 1] - 128.0;
			brgb[i * w * 3 + j * 3 + 2] = clamp(y + 1.402 * v); // R
			brgb[i * w * 3 + j * 3 + 1] = clamp(y - 0.344 * u - 0.714 * v); // G
			brgb[i * w * 3 + j * 3 + 0] = clamp(y + 1.772 * u); // B
#else // NTSC YUV to RGB
			int y, u, v;
			y = byuv[i * w + j] - 16;
			u = byuv[w * h + (i >> 1) * w + (j & ~0x1)] - 128;
			v = byuv[w * h + (i >> 1) * w + (j & ~0x1) + 1] - 128;
			brgb[i * w * 3 + j * 3 + 2] = clamp((298 * y + 409 * v + 128) >> 8);
			brgb[i * w * 3 + j * 3 + 1] = clamp((298 * y - 100 * u - 208 * v + 128) >> 8);
			brgb[i * w * 3 + j * 3 + 0] = clamp((298 * y + 516 * u + 128) >> 8);
#endif
		}
	}

	FreeImage_Initialise(FALSE);

	FIBITMAP *bitmap = FreeImage_ConvertFromRawBits(brgb, w, h, w * 3, 24, 0, 0, 0, TRUE);
	FreeImage_Save(FIF_JPEG, bitmap, njpg, JPEG_DEFAULT);
	FreeImage_Save(FIF_BMP, bitmap, nbmp, BMP_DEFAULT);
	FreeImage_Unload(bitmap);

	FreeImage_DeInitialise();

	free(brgb);
	free(byuv);
	fclose(fyuv);
	return 0;

err3:
	free(brgb);
err2:
	free(byuv);
err1:
	fclose(fyuv);
err0:
	return -1;
}

