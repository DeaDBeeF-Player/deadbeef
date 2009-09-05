apedir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = ape.la
ape_la_SOURCES = ape.c apewrapper.cpp apewrapper.h
ape_la_LDFLAGS = -module

ape_la_LIBADD = $(LDADD) $(APE_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
