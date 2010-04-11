if HAVE_SUPEREQ
supereqdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = supereq.la
supereq_la_SOURCES = supereq.c supereq.h Equ.cpp Fftsg_fl.cpp paramlist.hpp

supereq_la_LDFLAGS = -module

supereq_la_LIBADD = $(LDADD)
AM_CFLAGS = -std=c99
endif
