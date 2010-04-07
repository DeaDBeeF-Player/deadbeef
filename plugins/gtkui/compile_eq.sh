#!/bin/sh

#gcc g.c ddb-equalizer.c -g -O0 `pkg-config --libs --cflags gtk+-2.0 gdk-2.0`

valac -C -H graphic.h --library graphic graphic.vala --pkg=gtk+-2.0 &&
#gcc --shared -o libgraphic.so `pkg-config --libs --cflags gtk+-2.0 gdk-2.0` graphic.c &&
valac -C test.vala graphic.vapi --pkg=gtk+-2.0 &&
gcc test.c graphic.c -I. -o test `pkg-config --libs --cflags gtk+-2.0 gdk-2.0`
#    gcc -shared graphic.c -o graphic.so `pkg-config --libs --cflags gtk+-2.0 gdk-2.0`
#valac graphic.vala --pkg=gtk+-2.0
#gcc *.c `pkg-config --libs --cflags gtk+-2.0`

