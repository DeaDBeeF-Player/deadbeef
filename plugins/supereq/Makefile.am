if HAVE_SUPEREQ
pkglib_LTLIBRARIES = supereq.la
supereq_la_SOURCES = supereq.c Equ.cpp Equ.h Fftsg_fl.c paramlist.hpp

AM_CFLAGS = $(CFLAGS) -std=c99 -DUSE_OOURA
AM_CXXFLAGS = $(CXXFLAGS) -fno-exceptions -fno-rtti -fno-unwind-tables -DUSE_OOURA

supereq_la_LDFLAGS = -module -avoid-version $(NOCPPLIB)

supereq_la_LIBADD = $(LDADD)
endif
