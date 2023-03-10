if HAVE_MMS
pkglib_LTLIBRARIES = mms.la
mms_la_SOURCES = mmsplug.c\
	libmms/mms.c\
	libmms/mmsh.c\
	libmms/mmsx.c\
	libmms/uri.c\
	libmms/asfheader.h\
	libmms/bswap.h\
	libmms/mms-common.h\
	libmms/mms.h\
	libmms/mmsh.h\
	libmms/mmsio.h\
	libmms/mmsx.h\
	libmms/uri.h

mms_la_LDFLAGS = -module -avoid-version

mms_la_LIBADD = $(LDADD) $(LIBMMS_LIBS)
mms_la_CFLAGS = $(CFLAGS) -std=c99 -I@top_srcdir@/include
endif
