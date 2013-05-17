CC:=arm-none-linux-gnueabi-

emconfig:main.o command.o operator_lib.o operator_machine.o operator_app.o
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

clean:
	rm *.o
