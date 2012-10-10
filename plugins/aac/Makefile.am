if HAVE_AAC
aacpath=@top_srcdir@/plugins/aac
aacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = aac.la
aac_la_SOURCES = aac.c aac_parser.c aac_parser.h\
../libmp4ff/mp4atom.c\
../libmp4ff/mp4ff.c\
../libmp4ff/mp4meta.c\
../libmp4ff/mp4sample.c\
../libmp4ff/mp4tagupdate.c\
../libmp4ff/mp4util.c\
../libmp4ff/mp4ff.h\
../libmp4ff/mp4ffint.h\
../libmp4ff/mp4ff_int_types.h

aac_la_LDFLAGS = -module

aac_la_LIBADD = $(LDADD) $(FAAD2_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99 -DUSE_MP4FF -DUSE_TAGGING -I$(aacpath)/../libmp4ff
endif

