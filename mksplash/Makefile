CFLAGS=

all: mksplash

mksplash:mksplash.c 
	$(CROSS)gcc mksplash.c $(CFLAGS) -o mksplash $(EXTRA_LIBS)
	$(CROSS)strip -s mksplash

clean:
	rm -f *.o mksplash
