if HAVE_SHN
pkglib_LTLIBRARIES = ddb_shn.la
ddb_shn_la_SOURCES =\
	shnplugin.c\
	libshn/array.c\
	libshn/convert.c\
	libshn/misc.c\
	libshn/output.c\
	libshn/seek.c\
	libshn/shn.h\
	libshn/shorten.c\
	libshn/shorten.h\
	libshn/sulawalaw.c\
	libshn/vario.c\
	libshn/wave.c\
	libshn/bitshift.h

ddb_shn_la_LDFLAGS = -module -avoid-version

ddb_shn_la_LIBADD = $(LDADD) -lm
ddb_shn_la_CFLAGS = $(CFLAGS) -std=c99 -I@top_srcdir@/include -Ilibshn
endif
