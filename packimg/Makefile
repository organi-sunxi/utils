#!makefile

CROSS_COMPILE ?= arm-linux-gnueabi-

all: packimg packimg_burn

packimg: packimg.c
	gcc -g $< -o $@

packimg_burn: packimg_burn.c
	$(CROSS_COMPILE)gcc -g $< -o $@

clean:
	rm -rf packimg packimg_burn


