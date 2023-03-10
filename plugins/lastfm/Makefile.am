if HAVE_LASTFM
pkglib_LTLIBRARIES = lastfm.la
lastfm_la_SOURCES = lastfm.c
lastfm_la_LDFLAGS = -module -avoid-version

lastfm_la_LIBADD = $(LDADD) $(CURL_LIBS) $(DISPATCH_LIBS)
lastfm_la_CFLAGS = -std=c99 $(CURL_CFLAGS) $(DISPATCH_CFLAGS) -I@top_srcdir@/include
endif
