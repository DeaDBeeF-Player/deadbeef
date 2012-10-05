if HAVE_SHELLEXEC
shxdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = shellexec.la
shellexec_la_SOURCES = shellexec.c shellexec.h
shellexec_la_LDFLAGS = -module

shellexec_la_LIBADD = $(LDADD)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
