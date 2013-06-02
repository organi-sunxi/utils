all: lz4.o lz4hc.o

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o
