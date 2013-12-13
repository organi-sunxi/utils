#!makefile

CROSS_COMPILE ?= arm-linux-gnueabihf-
CFLAGS=

SRC=lcd-st7735.c
OBJ=$(SRC:.c=.o)

all: $(OBJ) lcd-st7735

%.o: %.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $< -o $@

lcd-st7735:$(OBJ)
	$(CROSS_COMPILE)gcc -o $@ $(OBJ)
	$(CROSS_COMPILE)strip -s $@

clean:
	rm -rf lcd-st7735 *.o

