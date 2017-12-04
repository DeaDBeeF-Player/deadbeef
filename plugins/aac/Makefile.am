if HAVE_AAC
pkglib_LTLIBRARIES = aac.la
aac_la_SOURCES = aac.c aac_parser.c aac_parser.h

aac_la_LDFLAGS = -module -avoid-version

aac_la_LIBADD = $(LDADD) $(FAAD2_LIBS) ../libmp4ff/libmp4ff.a ../../shared/libmp4tagutil.a
AM_CFLAGS = -I@top_srcdir@/plugins/libmp4ff $(CFLAGS) $(FAAD2_CFLAGS) -std=c99 -DUSE_MP4FF -DUSE_TAGGING
endif

