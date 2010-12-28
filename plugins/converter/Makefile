CONVERTER_OUT=converter.so
GUI_OUT=converter_gtkui.so

CC=gcc

CFLAGS+=-Wall -D_GNU_SOURCE -std=c99 -fPIC -g -I../..

LDFLAGS+=-module -shared

CONVERTER_SOURCES=converter.c
GUI_SOURCES=convgui.c interface.c support.c

CONVERTER_OBJECTS=$(CONVERTER_SOURCES:.c=.o)
GUI_OBJECTS=$(GUI_SOURCES:.c=.o)

all: $(CONVERTER_SOURCES) $(CONVERTER_OUT) $(GUI_SOURCES) $(GUI_OUT)

$(CONVERTER_OUT): $(CONVERTER_OBJECTS)
	$(CC) $(LDFLAGS) $(CONVERTER_OBJECTS) -o $@

GTK_CFLAGS=`pkg-config --cflags gtk+-2.0`
GTK_LIBS=`pkg-config --libs gtk+-2.0`

$(GUI_OUT): $(GUI_OBJECTS)
	$(CC) $(LDFLAGS) $(GUI_OBJECTS) $(GTK_LIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(GTK_CFLAGS) $< -c -o $@

clean:
	rm $(CONVERTER_OBJECTS) $(CONVERTER_OUT) $(GUI_OBJECTS) $(GUI_OUT)

