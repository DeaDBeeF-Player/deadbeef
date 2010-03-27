taglibdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = taglib.la
taglib_la_SOURCES = taglib.c

taglib_la_LDFLAGS = -module

taglib_la_LIBADD = $(LDADD) $(TAGLIB_DEPS_LIBS)
AM_CFLAGS = -std=c99 $(TAGLIB_DEPS_CFLAGS)
