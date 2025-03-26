if HAVE_LISTENBRAINZ
pkglib_LTLIBRARIES = listenbrainz.la
listenbrainz_la_SOURCES = listenbrainz.c
listenbrainz_la_LDFLAGS = -module -avoid-version

listenbrainz_la_LIBADD = $(LDADD) $(CURL_LIBS) $(DISPATCH_LIBS) $(JANSSON_LIBS)
listenbrainz_la_CFLAGS = -std=c99 $(CURL_CFLAGS) $(DISPATCH_CFLAGS) $(JANSSON_CFLAGS) -I@top_srcdir@/include
endif
