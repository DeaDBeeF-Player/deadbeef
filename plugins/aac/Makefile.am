if HAVE_AAC
aacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = aac.la
aac_la_SOURCES = aac.c aac_parser.c aac_parser.h

aac_la_LDFLAGS = -module

aac_la_LIBADD = $(LDADD) $(FAAD2_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif

