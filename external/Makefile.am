noinst_LTLIBRARIES = libmp4p.la
libmp4p_la_SOURCES = mp4p/src/mp4p.c mp4p/src/mp4patomdata.c mp4p/src/mp4pfile.c\
				   	mp4p/include/mp4p/mp4p.h mp4p/include/mp4p/mp4patomdata.h mp4p/include/mp4p/mp4pfile.h 
libmp4p_la_CFLAGS = -fPIC -std=c99 -I@top_srcdir@/external/mp4p/include


if HAVE_PIPEWIRE
pkglib_LTLIBRARIES = ddb_out_pw.la
ddb_out_pw_la_SOURCES = ddb_output_pw/pw.c
ddb_out_pw_la_LDFLAGS = -module -avoid-version
ddb_out_pw_la_CFLAGS = $(CFLAGS) $(PIPEWIRE_DEPS_CFLAGS) -std=c99
ddb_out_pw_la_LIBADD = $(LDADD) $(PIPEWIRE_DEPS_LIBS)
endif
