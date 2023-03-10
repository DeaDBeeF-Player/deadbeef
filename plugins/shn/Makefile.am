if HAVE_SHN
pkglib_LTLIBRARIES = ddb_shn.la
ddb_shn_la_SOURCES = array.c convert.c misc.c output.c seek.c shn.c shn.h shorten.c shorten.h sulawalaw.c vario.c wave.c bitshift.h

ddb_shn_la_LDFLAGS = -module -avoid-version

ddb_shn_la_LIBADD = $(LDADD) -lm
ddb_shn_la_CFLAGS = $(CFLAGS) -std=c99 -I@top_srcdir@/include
endif
