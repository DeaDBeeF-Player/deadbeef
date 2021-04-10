if HAVE_FLAC
pkglib_LTLIBRARIES = flac.la
flac_la_SOURCES = flac.c
flac_la_LDFLAGS = -module -avoid-version -export-symbols-regex flac_load

if HAVE_OGG
oggedit_def = -DUSE_OGGEDIT=1
oggedit_lib = ../liboggedit/liboggedit.la $(OGG_LIBS)
endif

flac_la_LIBADD = $(LDADD) $(FLAC_LIBS) $(oggedit_lib)
AM_CFLAGS = $(CFLAGS) $(FLAC_CFLAGS) $(oggedit_def) -std=c99
endif
