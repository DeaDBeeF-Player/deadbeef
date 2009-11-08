if HAVE_CURL
lastfmdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = lastfm.la
lastfm_la_SOURCES = lastfm.c
lastfm_la_LDFLAGS = -module

lastfm_la_LIBADD = $(LDADD) $(CURL_LIBS)
AM_CFLAGS = -std=c99
endif
