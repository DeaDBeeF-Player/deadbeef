if HAVE_SNDFILE
pkglib_LTLIBRARIES = sndfile.la
sndfile_la_SOURCES = sndfile.c
sndfile_la_LDFLAGS = -module -avoid-version

sndfile_la_LIBADD = $(LDADD) $(SNDFILE_LIBS)
sndfile_la_CFLAGS = $(CFLAGS) $(SNDFILE_CFLAGS) -std=c99 -I@top_srcdir@/include
endif
