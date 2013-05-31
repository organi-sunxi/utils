
#CC=arm-linux-gnueabi-
CC=arm-unknown-linux-uclibcgnueabi-
CFLAGS=-DINITROOT_STARTUP
LDFLAGS=-static

EXEC= initroot_startup

all: $(EXEC) main

$(EXEC): initroot_startup.c initroot_startup.h
	$(CC)gcc $(EXEC).c $(LDFLAGS) $(CFLAGS) -c -o $(EXEC).o
	cp $(EXEC).o lib$(EXEC).a
#	$(CC)ar c lib$(EXEC).a $(EXEC).o

main: main.cpp
	$(CC)g++ main.cpp $(LDFLAGS) $(CFLAGS) -o init -linitroot_startup -L./
	$(CC)strip -S init

clean:
	rm -f *.o $(EXEC) *.a init
