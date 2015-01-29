#!makefile

CROSS_COMPILE ?=
CC ?= $(CROSS_COMPILE)gcc
STRIP ?= $(CROSS_COMPILE)strip

CFLAGS=

SRC=packimg_burn.c packimg.c unpackimg.c
OBJ=$(SRC:.c=.o)

all: $(OBJ) packimg

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

packimg:$(OBJ)
	$(CC) -o $@ $(OBJ)
	$(STRIP) -s $@
	rm -f unpackimg && ln -s packimg unpackimg
	rm -f packimg_burn && ln -s packimg packimg_burn

clean:
	rm -rf packimg unpackimg packimg_burn *.o

