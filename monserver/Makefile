
KERNEL_PATH ?= ../../linux-sunxi
CROSS_COMPILE ?= arm-none-linux-gnueabi-
CC ?= $(CROSS_COMPILE)gcc
STRIP ?= $(CROSS_COMPILE)strip

EXTRA_LIBS += -lpthread
CFLAGS = -Wall
LDFLAGS =

EXEC = monserver

all: $(EXEC) client

$(EXEC): main.c video.c enum.c
	$(CC) $^ $(LDFLAGS) $(CFLAGS) -I$(KERNEL_PATH)/include -o $@ $(EXTRA_LIBS)
	$(STRIP) $(EXEC)

client: testclt.c monclient.c
	gcc $^ $(LDFLAGS) $(CFLAGS) -o montest.host $(EXTRA_LIBS)
	$(CC) $^ $(LDFLAGS) $(CFLAGS) -o montest $(EXTRA_LIBS)
	$(STRIP) montest

clean:
	rm -f *.o a.out $(EXEC) *.gdb montest montest.host enumd
