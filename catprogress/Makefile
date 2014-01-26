#!makefile

CROSS_COMPILE ?= arm-none-linux-gnueabi-
CFLAGS=

EXEC=catprogress
SRC=catprogress.c
OBJ=$(SRC:.c=.o)

all: $(OBJ) $(EXEC)

%.o: %.c
	$(CROSS_COMPILE)gcc -c $(CFLAGS) $< -o $@

$(EXEC):$(OBJ)
	$(CROSS_COMPILE)gcc -o $@ $(OBJ)
	$(CROSS_COMPILE)strip -s $@

clean:
	rm -rf $(EXEC) *.o

