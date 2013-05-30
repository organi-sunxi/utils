CC:=arm-none-linux-gnueabi-

emconfig:defaultenv.h main.o command.o operator_lib.o operator_machine.o operator_app.o
	$(CC)gcc -o emconfig main.o command.o operator_lib.o \
			operator_machine.o operator_app.o

main.o:main.c
	$(CC)gcc -c main.c

command.o:command.c command.h
	$(CC)gcc -c command.c

operator_lib.o:operator_lib.c
	$(CC)gcc -c operator_lib.c

operator_machine.o:operator_machine.c
	$(CC)gcc -c operator_machine.c

operator_app.o:operator_app.c
	$(CC)gcc -c operator_app.c

defaultenv.h:emconfig.conf
	echo "#ifndef DEFAULTENV_H" > $@
	echo "#define DEFAULTENV_H\n" >> $@
	echo "//This file is generated form emconfig.conf. don't edit it!\n" >> $@
	sed /^[[:space:]]*$$/d emconfig.conf | sed '/#/d' | sed 's/=/\t\t\"/' | sed 's/^/#define &/g' | sed 's/$$/\"/g' >> $@
	echo "\n#endif" >> $@

clean:
	rm *.o defaultenv.h
