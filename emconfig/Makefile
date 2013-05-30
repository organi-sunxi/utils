CC:=arm-none-linux-gnueabi-
SRC=main.c command.c operator_lib.c operator_machine.c operator_app.c \
	em6000.c

OBJ=$(SRC:.c=.o)
DEP=$(SRC:.c=.dep)

all: defaultenv.h $(OBJ) emconfig

%.o: %.c
	$(CC)gcc -c $(CFLAGS) $< -o $@

%.dep: %.c
	$(CC)gcc -MM $(CFLAGS) $< > $@

emconfig:$(OBJ)
	$(CC)gcc -o $@ $(OBJ)
	$(CC)strip -s $@

defaultenv.h:emconfig.conf
	echo "#ifndef DEFAULTENV_H" > $@
	echo "#define DEFAULTENV_H\n" >> $@
	echo "//This file is generated form emconfig.conf. don't edit it!\n" >> $@
	sed /^[[:space:]]*$$/d emconfig.conf | sed '/#/d' | sed 's/=/\t\t\"/' | sed 's/^/#define &/g' | sed 's/$$/\"/g' >> $@
	echo "\n#endif" >> $@

clean:
	rm -f *.o defaultenv.h *.dep
