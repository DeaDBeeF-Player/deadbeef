if HAVE_SHELLEXEC
pkglib_LTLIBRARIES = shellexec.la
shellexec_la_SOURCES = shellexec.c shellexec.h shellexecutil.c shellexecutil.h
shellexec_la_LDFLAGS = -module -avoid-version

shellexec_la_LIBADD = $(LDADD) $(JANSSON_LIBS)
shellexec_la_CFLAGS = -std=c99 $(JANSSON_CFLAGS) -I@top_srcdir@/include
endif
