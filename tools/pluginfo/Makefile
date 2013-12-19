CC=gcc
CFLAGS=-Wall
LDFLAGS=-lpthread -ldl -lm

all:
	mkdir x86_64
	mkdir i686
	$(CC) -m32 $(CFLAGS) pluginfo.c $(LDFLAGS) -o i686/pluginfo
	$(CC) -m64 $(CFLAGS) pluginfo.c $(LDFLAGS) -o x86_64/pluginfo

clean:
	rm *.o pluginfo
