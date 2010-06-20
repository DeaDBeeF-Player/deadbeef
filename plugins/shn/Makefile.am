if HAVE_SHN
shndir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = shn.la
shn_la_SOURCES = array.c config.h convert.c misc.c output.c seek.c shn.c shn.h shorten.c shorten.h sulawalaw.c vario.c wave.c

shn_la_LDFLAGS = -module

shn_la_LIBADD = $(LDADD)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
