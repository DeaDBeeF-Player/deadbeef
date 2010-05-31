if HAVE_WILDMIDI
wildmidipath=@top_srcdir@/plugins/wildmidi

EXTRA_DIST = $(wildmidipath)/README

pkglib_LTLIBRARIES = wildmidi.la

wildmidi_la_SOURCES = wildmidiplug.c src/wildmidi_lib.c include/wildmidi_lib.h

wildmidi_la_LDFLAGS = -module

AM_CFLAGS = $(CFLAGS) -std=c99 -I$(wildmidipath)/include -DWILDMIDI_VERSION=\"0.2.2\" -DWILDMIDILIB_VERSION=\"0.2.2\" -DTIMIDITY_CFG=\"/etc/timidity.conf\" -fPIC
endif
