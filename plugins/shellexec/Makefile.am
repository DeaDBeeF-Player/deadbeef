if HAVE_SHELLEXEC
pkglib_LTLIBRARIES = shellexec.la
shellexec_la_SOURCES = shellexec.c shellexec.h
shellexec_la_LDFLAGS = -module -avoid-version

shellexec_la_LIBADD = $(LDADD)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
