ossdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = oss.la
oss_la_SOURCES = oss.c
oss_la_LDFLAGS = -module
include /etc/oss.conf
AM_CFLAGS = $(CFLAGS) -I$(OSSLIBDIR)/include/sys -std=c99
