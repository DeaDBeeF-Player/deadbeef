if HAVE_MONO2STEREO
pkglib_LTLIBRARIES = ddb_mono2stereo.la
ddb_mono2stereo_la_SOURCES = mono2stereo.c
ddb_mono2stereo_la_LDFLAGS = -module -avoid-version

ddb_mono2stereo_la_LIBADD = $(LDADD)
ddb_mono2stereo_la_CFLAGS = $(CFLAGS) -std=c99 -I@top_srcdir@/include
endif
