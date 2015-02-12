
#CC=arm-linux-gnueabi-
CROSS_COMPILE ?= arm-linux-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar
CFLAGS += -DINITROOT_STARTUP
LDFLAGS += -static

EXEC = initroot_startup
LIB = lib$(EXEC).a

all: $(LIB) init

$(LIB): $(EXEC).c $(EXEC).h
	$(CC) $(EXEC).c $(CFLAGS) -c -o $(EXEC).o
	$(AR) cr $(LIB) $(EXEC).o

init: main.cpp
	$(CXX) $< $(LDFLAGS) $(CFLAGS) -o $@ -l$(EXEC) -L./

install: $(LIB)
	install -d $(DESTDIR)/usr/lib
	install -m 755 $(LIB) $(DESTDIR)/usr/lib

clean:
	rm -f *.o *.a init
