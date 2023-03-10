if HAVE_AAC
pkglib_LTLIBRARIES = aac.la
aac_la_SOURCES =\
	aac.c\
	aac_decoder_faad2.c\
	aac_decoder_wrap.c\
	aac_parser.c\
	aac_decoder_faad2.h\
	aac_decoder_protocol.h\
	aac_parser.h

aac_la_LDFLAGS = -module -avoid-version

aac_la_LIBADD = $(LDADD) $(FAAD2_LIBS) ../../shared/libmp4tagutil.la ../../external/libmp4p.la
aac_la_CFLAGS = -I@top_srcdir@/external/mp4p/include -I@top_srcdir@/include $(CFLAGS) $(FAAD2_CFLAGS) -std=c99
endif

