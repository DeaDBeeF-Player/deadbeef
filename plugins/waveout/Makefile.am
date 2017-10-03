
if HAVE_WAVEOUT
pkglib_LTLIBRARIES = waveout.la
AM_CFLAGS = $(CFLAGS) -std=c99
waveout_la_SOURCES = waveout.c
waveout_la_LDFLAGS = -module -avoid-version
waveout_la_LIBADD = $(LDADD) $(WAVEOUT_LIBS) -lwinmm -lksuser
endif
