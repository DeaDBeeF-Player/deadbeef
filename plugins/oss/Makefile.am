if HAVE_OSS
pkglib_LTLIBRARIES = oss.la
AM_CFLAGS = $(CFLAGS) $(OSS_CFLAGS) -std=c99
oss_la_SOURCES = oss.c
oss_la_LDFLAGS = -module -avoid-version
oss_la_LIBADD = ${OSS_LIBS}
endif
