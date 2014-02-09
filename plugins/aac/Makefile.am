if HAVE_AAC
aacpath=@top_srcdir@/plugins/aac
aacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = aac.la
aac_la_SOURCES = aac.c aac_parser.c aac_parser.h
aac_la_LDFLAGS = -module -avoid-version

aac_la_LIBADD = $(LDADD) $(FAAD2_LIBS) ../libmp4ff/libmp4ff.a
AM_CFLAGS = $(CFLAGS) $(FAAD2_CFLAGS) -std=c99 -DUSE_MP4FF -DUSE_TAGGING -I@top_srcdir@/plugins/libmp4ff
endif

