hotkeysdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = hotkeys.la
hotkeys_la_SOURCES = hotkeys.c
hotkeys_la_LDFLAGS = -module

EXTRA_hotkeys_la_SOURCES = keysyms.inc

hotkeys_la_LIBADD = $(LDADD) -lX11
AM_CFLAGS = $(CFLAGS) -std=c99
