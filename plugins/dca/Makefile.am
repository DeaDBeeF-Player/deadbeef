if HAVE_DCA

pkglib_LTLIBRARIES = dca.la

dca_la_SOURCES = dcaplug.c\
extract_dca.c\
getopt.c\
gettimeofday.c\
parse.c\
bitstream.c\
downmix.c\
convert2s16.c\
plugins/dca/audio_out.h\
plugins/dca/dca.h\
plugins/dca/dts.h\
plugins/dca/getopt.h\
plugins/dca/gettimeofday.h\
plugins/dca/tendra.h\
plugins/dca/dca_internal.h\
plugins/dca/tables_adpcm.h\
plugins/dca/tables_fir.h\
plugins/dca/tables.h\
plugins/dca/tables_huffman.h\
plugins/dca/tables_quantization.h\
plugins/dca/tables_vq.h

dca_la_LDFLAGS = -module

AM_CFLAGS = $(CFLAGS) -fPIC

endif
