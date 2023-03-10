if HAVE_ALAC
pkglib_LTLIBRARIES = alac.la
alac_la_SOURCES = alac_plugin.c\
	alac.c decomp.h\
	../../shared/mp4tagutil.c ../../shared/mp4tagutil.h

alac_la_LDFLAGS = -module -avoid-version

alac_la_LIBADD = $(LDADD) $(FAAD2_LIBS) ../../external/libmp4p.la
alac_la_CFLAGS = $(CFLAGS) -std=c99 -I@top_srcdir@/external/mp4p/include -I@top_srcdir@/include
endif


