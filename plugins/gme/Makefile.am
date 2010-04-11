gmepath=@top_srcdir@/plugins/gme/Game_Music_Emu-0.5.2
SUBDIRS = Game_Music_Emu-0.5.2 Game_Music_Emu-0.5.2/gme

gmedir = $(libdir)/$(PACKAGE)

pkglib_LTLIBRARIES = gme.la

gme_la_SOURCES = cgme.c
gme_la_LDFLAGS = -module

gme_la_LIBADD = $(LDADD) $(gmepath)/gme/libgme.a
AM_CFLAGS = $(CFLAGS) -I$(gmepath) -std=c99 -fPIC
