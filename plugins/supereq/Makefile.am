if HAVE_SUPEREQ
supereqdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = supereq.la
supereq_la_SOURCES = supereq.c Equ.cpp Equ.h Fftsg_fl.c paramlist.hpp

AM_CFLAGS = $(CFLAGS) -std=c99 -DUSE_OOURA
AM_CPPFLAGS = $(CXXFLAGS) -fno-exceptions -fno-rtti -nostdlib -fno-unwind-tables -DUSE_OOURA

supereq_la_LDFLAGS = -module -nostdlib -lsupc++

supereq_la_LIBADD = $(LDADD)
endif
