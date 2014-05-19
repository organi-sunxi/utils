
CC ?= arm-none-linux-gnueabi-
EXTRA_LIBS += -lpthread
CFLAGS = -Wall
LDFLAGS =

EXEC = monserver

all: $(EXEC) client

$(EXEC): main.c video.c enum.c
	$(CC)gcc $^ $(LDFLAGS) $(CFLAGS) -o $@ $(EXTRA_LIBS)
	$(CC)strip $(EXEC)

client: testclt.c monclient.c
	gcc $^ $(LDFLAGS) $(CFLAGS) -o montest.host $(EXTRA_LIBS)
	$(CC)gcc $^ $(LDFLAGS) $(CFLAGS) -o montest $(EXTRA_LIBS)
	$(CC)strip montest

clean:
	rm -f *.o a.out $(EXEC) *.gdb montest montest.host enumd
