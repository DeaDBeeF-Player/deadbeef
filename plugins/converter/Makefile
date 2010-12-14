OUT=converter.so

CC=gcc

CFLAGS+=-Wall -D_GNU_SOURCE -std=c99 -I../..

LDFLAGS+=-module -shared -fPIC

SOURCES=converter.c

OBJECTS=$(SOURCES:.c=.o)

all: $(SOURCES) $(OUT)

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm $(OBJECTS) $(OUT)

