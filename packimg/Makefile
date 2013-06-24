#!makefile

CROSS_COMPILE ?=
CFLAGS=

SRC=packimg_burn.c packimg.c unpackimg.c
OBJ=$(SRC:.c=.o)

all: $(OBJ) packimg

%.o: %.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $< -o $@

packimg:$(OBJ)
	$(CROSS_COMPILE)gcc -o $@ $(OBJ)
	$(CROSS_COMPILE)strip -s $@
	ln -s packimg unpackimg
	ln -s packimg packimg_burn

clean:
	rm -rf packimg unpackimg packimg_burn *.o

