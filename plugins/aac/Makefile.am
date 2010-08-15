if HAVE_AAC
aacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = aac.la
aac_la_SOURCES = aac.c aac_parser.c aac_parser.h\
mp4ff/mp4atom.c\
mp4ff/mp4ff.c\
mp4ff/mp4meta.c\
mp4ff/mp4sample.c\
mp4ff/mp4tagupdate.c\
mp4ff/mp4util.c\
mp4ff/mp4ff.h\
mp4ff/mp4ffint.h\
mp4ff/mp4ff_int_types.h

aac_la_LDFLAGS = -module

aac_la_LIBADD = $(LDADD) $(FAAD2_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99 -Imp4ff
endif

