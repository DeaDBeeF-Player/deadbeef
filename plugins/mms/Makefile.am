if HAVE_MMS
mmsdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = mms.la
mms_la_SOURCES = mms.c
mms_la_LDFLAGS = -module

mms_la_LIBADD = $(LDADD) $(LIBMMS_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
