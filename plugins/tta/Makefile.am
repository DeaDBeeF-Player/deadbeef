EXTRA_DIST = $(ttapath)/README

if HAVE_TTA
pkglib_LTLIBRARIES = tta.la

tta_la_SOURCES =\
	ttaplug.c\
	libtta/filter.h\
	libtta/ttadec.c\
	libtta/ttadec.h

tta_la_LDFLAGS = -module -avoid-version

ttapath=@top_srcdir@/plugins/tta
tta_la_CFLAGS = $(CFLAGS) -std=c99 -fPIC -I@top_srcdir@/include -I$(ttapath)/libtta
endif

