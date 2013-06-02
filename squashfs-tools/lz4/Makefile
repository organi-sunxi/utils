all: lz4.o lz4hc.o

lz4.o: lz4.c lz4_decoder.h  lz4def.h  lz4_encoder.h  lz4.h
lz4hc.o: lz4hc.c lz4hc_decoder.h  lz4def.h  lz4_encoder.h  lz4.h
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o
