CC=gcc
CFLAGS=-Wall -I../../include
LDFLAGS=-lpthread -ldl -lm

all:
	mkdir -p x86_64
	$(CC) -m64 $(CFLAGS) pluginfo.c $(LDFLAGS) -o x86_64/pluginfo
#	mkdir -p i686
#	-$(CC) -m32 $(CFLAGS) pluginfo.c $(LDFLAGS) -o i686/pluginfo

clean:
	rm *.o pluginfo
