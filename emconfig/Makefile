CROSS_COMPILE ?= arm-none-linux-gnueabi-
CC ?= $(CROSS_COMPILE)gcc
STRIP ?= $(CROSS_COMPILE)strip

SRC = main.c command.c operator_lib.c operator_machine.c operator_app.c lib.c \
	em5000.c em6000.c

OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.dep)

all: defaultenv.h $(OBJ) emconfig

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.dep: %.c
	$(CC) -MM $(CFLAGS) $< > $@

emconfig:$(OBJ)
	$(CC) -o $@ $(OBJ)
	$(STRIP) -s $@

defaultenv.h:emconfig.conf
	echo "#ifndef DEFAULTENV_H" > $@
	printf "#define DEFAULTENV_H\n\n" >> $@
	echo "//This file is generated form emconfig.conf. don't edit it!\n" >> $@
	sed /^[[:space:]]*$$/d emconfig.conf | sed '/#/d' | sed 's/=/\t\t\"/' | sed 's/^/#define &/g' | sed 's/$$/\"/g' >> $@
	printf "\n#endif\n" >> $@

clean:
	rm -f *.o defaultenv.h *.dep
