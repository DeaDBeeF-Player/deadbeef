lib_LTLIBRARIES = libwavpack.la

libwavpack_la_SOURCES = \
	bits.c \
	float.c \
	metadata.c \
	unpack.c \
	unpack3.c \
	wputils.c \
	words.c \
	extra1.c \
	extra2.c \
	pack.c \
	tags.c

noinst_HEADERS = \
	unpack3.h \
	wavpack_local.h \
	wavpack_version.h

libwavpack_la_CFLAGS = $(AM_CFLAGS) $(ICONV_CFLAGS)
libwavpack_la_LIBADD = $(AM_LDADD) $(LIBM) $(ICONV_LIBS)
libwavpack_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -export-symbols-regex '^Wavpack.*$$' -no-undefined

MAINTAINERCLEANFILES = \
	Makefile.in
