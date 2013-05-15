#arm-none-linux-gnueabi-
emconfig:main.o command.o operator_lib.o operator_machine.o operator_app.o
	gcc -o emconfig main.o command.o operator_lib.o \
			operator_machine.o operator_app.o

main.o:main.c
	gcc -c main.c

command.o:command.c command.h
	gcc -c command.c

operator_lib.o:operator_lib.c operator_lib.h
	gcc -c operator_lib.c

operator_machine.o:operator_machine.c operator_machine.h
	gcc -c operator_machine.c

operator_app.o:operator_app.c operator_app.h
	gcc -c operator_app.c

clean:
	rm *.o
