if HAVE_ALAC
pkglib_LTLIBRARIES = alac.la
alac_la_SOURCES = alac_plugin.c\
	alac.c decomp.h demux.c demux.h stream.c stream.h\
	../../shared/mp4tagutil.c ../../shared/mp4tagutil.h

alac_la_LDFLAGS = -module -avoid-version

alac_la_LIBADD = $(LDADD) $(FAAD2_LIBS) ../libmp4ff/libmp4ff.la
AM_CFLAGS = $(CFLAGS) -std=c99 -DUSE_MP4FF -DUSE_TAGGING -I@top_srcdir@/plugins/libmp4ff
endif


