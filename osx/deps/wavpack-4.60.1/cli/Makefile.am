bin_PROGRAMS = wavpack wvunpack wvgain

wavpack_SOURCES = wavpack.c utils.c md5.c
wavpack_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/include
wavpack_LDADD = $(AM_LDADD) $(top_builddir)/src/.libs/libwavpack.la $(LIBM)

wvunpack_SOURCES = wvunpack.c utils.c md5.c
wvunpack_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/include
wvunpack_LDADD = $(AM_LDADD) $(top_builddir)/src/.libs/libwavpack.la $(LIBM)

wvgain_SOURCES = wvgain.c utils.c
wvgain_CFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/include
wvgain_LDADD = $(AM_LDADD) $(top_builddir)/src/.libs/libwavpack.la $(LIBM)

noinst_HEADERS = \
	utils.h \
	md5.h

MAINTAINERCLEANFILES = \
	Makefile.in
