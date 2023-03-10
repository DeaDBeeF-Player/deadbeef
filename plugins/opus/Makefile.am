if HAVE_OPUS
pkglib_LTLIBRARIES = opus.la
opus_la_SOURCES = opus.c
opus_la_LDFLAGS = -module -avoid-version -lm -export-symbols-regex opus_load

opus_la_LIBADD = $(LDADD) $(OPUS_LIBS) ../liboggedit/liboggedit.la
opus_la_CFLAGS = $(CFLAGS) $(OPUS_CFLAGS) -std=c99 -I@top_srcdir@/include
endif
