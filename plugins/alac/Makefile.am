if HAVE_ALAC
alacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = alac.la
alac_la_SOURCES = alac_plugin.c\
alac.c decomp.h demux.c demux.h stream.c stream.h\
../libmp4ff/mp4atom.c\
../libmp4ff/mp4ff.c\
../libmp4ff/mp4meta.c\
../libmp4ff/mp4sample.c\
../libmp4ff/mp4tagupdate.c\
../libmp4ff/mp4util.c\
../libmp4ff/mp4ff.h\
../libmp4ff/mp4ffint.h\
../libmp4ff/mp4ff_int_types.h

alac_la_LDFLAGS = -module

alac_la_LIBADD = $(LDADD) $(FAAD2_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99 -DUSE_MP4FF -DUSE_TAGGING -I../libmp4ff
endif


