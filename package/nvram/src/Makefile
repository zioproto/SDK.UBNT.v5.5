# $Id: Makefile 10749 2008-04-07 00:24:19Z nbd $

LIBNVRAM_OBJS := nvram.o

all: libnvram.so nvram

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -I. -Iinclude -o $@ $^

libnvram.so: $(LIBNVRAM_OBJS)
	$(CC) -shared -o $@ $^

nvram: main.o
	$(CC) -o $@ $^ -L. -lnvram

clean:
	rm -f *.o *.so nvram
