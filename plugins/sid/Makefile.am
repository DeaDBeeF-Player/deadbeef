sidpath=@top_srcdir@/plugins/sid/sidplay-libs
SUBDIRS = sidplay-libs

pkglib_LTLIBRARIES = sid.la
sid_la_SOURCES = plugin.c csid.cpp csid.h
	
sid_la_LIBADD =  $(sidpath)/libsidplay2.a -lstdc++
sid_la_LDFLAGS = -module

AM_CFLAGS = $(CFLAGS) -std=c99 -I$(sidpath)/libsidplay/include -I$(sidpath)/builders/resid-builder/include -fPIC
AM_CPPFLAGS = $(CXXFLAGS) -I$(sidpath)/libsidplay/include -I$(sidpath)/builders/resid-builder/include -fPIC

