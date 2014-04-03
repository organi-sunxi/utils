LINUX_INCLUDE=/data/samba/armlinux/omap3/uptech/linux-2.6/kernel/linux-2.6.32/include

CC=arm-none-linux-gnueabi-
#CC=arm-unknown-linux-uclibcgnueabi-
EXTRA_LIBS += -lpthread
CFLAGS=-Wall
LDFLAGS=

EXEC= monserver

all: $(EXEC) client

$(EXEC): main.c video.c enum.c
	$(CC)gcc -I$(LINUX_INCLUDE) $^ $(LDFLAGS) $(CFLAGS) -o $@ $(EXTRA_LIBS)
	$(CC)strip $(EXEC)

client: testclt.c monclient.c
	gcc $^ $(LDFLAGS) $(CFLAGS) -o montest.host $(EXTRA_LIBS)
	$(CC)gcc $^ $(LDFLAGS) $(CFLAGS) -o montest $(EXTRA_LIBS)
	$(CC)strip montest

clean:
	rm -f *.o a.out $(EXEC) *.gdb montest montest.host enumd
