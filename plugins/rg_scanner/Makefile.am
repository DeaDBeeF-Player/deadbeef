if HAVE_RGSCANNER
pkglib_LTLIBRARIES = rg_scanner.la
rg_scanner_la_SOURCES = rg_scanner.c rg_scanner.h ebur128/ebur128.c ebur128/ebur128.h
rg_scanner_la_LDFLAGS = -module -avoid-version

rg_scanner_la_LIBADD = $(LDADD) $(DISPATCH_LIBS)
rg_scanner_la_CFLAGS = -std=c99 $(DISPATCH_CFLAGS) -I@top_srcdir@/include
if HAVE_SSE2
rg_scanner_la_CFLAGS += -msse2 -mfpmath=sse
endif
endif
