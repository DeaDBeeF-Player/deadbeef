if HAVE_TTA
ttapath=@top_srcdir@/plugins/tta

EXTRA_DIST = $(ttapath)/README

pkglib_LTLIBRARIES = tta.la

tta_la_SOURCES = ttaplug.c filter.h ttadec.c ttadec.h

tta_la_LDFLAGS = -module

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC
endif

