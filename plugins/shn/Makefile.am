if HAVE_SHN
shndir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = ddb_shn.la
ddb_shn_la_SOURCES = array.c convert.c misc.c output.c seek.c shn.c shn.h shorten.c shorten.h sulawalaw.c vario.c wave.c bitshift.h

ddb_shn_la_LDFLAGS = -module

ddb_shn_la_LIBADD = $(LDADD) -lm
AM_CFLAGS = $(CFLAGS) -std=c99
endif
