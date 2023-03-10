if HAVE_WAVPACK
pkglib_LTLIBRARIES = wavpack.la
wavpack_la_SOURCES = wavpack.c
wavpack_la_LDFLAGS = -module -avoid-version

wavpack_la_LIBADD = $(LDADD) $(WAVPACK_LIBS)
wavpack_la_CFLAGS = $(CFLAGS) $(WAVPACK_CFLAGS) -std=c99 -I@top_srcdir@/include
endif
