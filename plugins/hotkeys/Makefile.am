hotkeysdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = hotkeys.la
hotkeys_la_SOURCES = hotkeys.c
hotkeys_la_LDFLAGS = -module

EXTRA_hotkeys_la_SOURCES = keysyms.inc

hotkeys_la_LIBADD = $(LDADD) $(hotkeys_DEPS_LIBS)
AM_CFLAGS = $(hotkeys_DEPS_CFLAGS) -std=c99
