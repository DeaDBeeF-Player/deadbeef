if HAVE_HOTKEYS
pkglib_LTLIBRARIES = hotkeys.la
hotkeys_la_SOURCES = hotkeys.c hotkeys.h actionhandlers.c actionhandlers.h
hotkeys_la_LDFLAGS = -module -avoid-version

EXTRA_hotkeys_la_SOURCES = keysyms.inc

hotkeys_la_LIBADD = $(LDADD) $(HOTKEYS_LIBS) ../libparser/libparser.a
AM_CFLAGS = $(CFLAGS) $(HOTKEYS_CFLAGS) -std=c99
endif
