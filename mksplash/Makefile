CROSS_COMPILE ?= 
CC ?= $(CROSS_COMPILE)gcc
STRIP ?= $(CROSS_COMPILE)strip

all: mksplash

mksplash: mksplash.c 
	$(CC) mksplash.c $(CFLAGS) -o mksplash
	$(STRIP) -s mksplash

clean:
	rm -f *.o mksplash
