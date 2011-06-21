CC?=gcc

CFLAGS+=-Wall -fPIC -D_GNU_SOURCE -std=c99

LDFLAGS+=-module -shared -lm

SOURCES=mono2stereo.c

OBJECTS=$(SOURCES:.c=.o)

OUT?=ddb_mono2stereo.so

all: $(SOURCES) $(OUT)

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm $(OBJECTS) $(OUT)

