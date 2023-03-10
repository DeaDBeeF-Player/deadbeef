if HAVE_M3U
pkglib_LTLIBRARIES = m3u.la

m3u_la_SOURCES = m3u.c

m3u_la_LDFLAGS = -module -avoid-version -lm

m3u_la_LIBADD = $(LIBADD)

m3u_la_CFLAGS = $(CFLAGS) -I@top_srcdir@/include -std=c99
endif
