if HAVE_MPRIS

mpris_path=@top_srcdir@/plugins/mpris

EXTRA_DIST = $(mpris_path)/COPYING

pkglib_LTLIBRARIES = mpris.la

mpris_la_SOURCES = mpris.c mprisServer.c logging.h mprisServer.h
mpris_la_LDFLAGS = -module -avoid-version
mpris_la_LIBADD = $(LDADD) $(MPRIS_LIBS)
mpris_la_CFLAGS = $(CFLAGS) -std=c99 $(MPRIS_CFLAGS) -I@top_srcdir@/include -fPIC

endif
