if HAVE_OSS
pkglib_LTLIBRARIES = oss.la
oss_la_CFLAGS = $(CFLAGS) $(OSS_CFLAGS) -std=c99 -I@top_srcdir@/include
oss_la_SOURCES = oss.c
oss_la_LDFLAGS = -module -avoid-version
oss_la_LIBADD = ${OSS_LIBS}
endif
