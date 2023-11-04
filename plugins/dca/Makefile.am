if HAVE_DCA

pkglib_LTLIBRARIES = dca.la

dca_la_SOURCES = dcaplug.c\
	libdca/audio_out.h\
	libdca/bitstream.c\
	libdca/bitstream.h
	libdca/dca.h\
	libdca/dca_internal.h\
	libdca/downmix.c\
	libdca/dts.h\
	libdca/extract_dca.c\
	libdca/gettimeofday.c\
	libdca/gettimeofday.h\
	libdca/parse.c\
	libdca/tables.h\
	libdca/tables_adpcm.h\
	libdca/tables_fir.h\
	libdca/tables_huffman.h\
	libdca/tables_quantization.h\
	libdca/tables_vq.h\
	libdca/tendra.h

dca_la_LDFLAGS = -module -avoid-version
dca_la_LIBADD = $(LDADD) -lm

dca_la_CFLAGS = $(CFLAGS) -fPIC -std=c99 -I@top_srcdir@/include -Ilibdca

endif
