if HAVE_DCA

pkglib_LTLIBRARIES = dca.la

dca_la_SOURCES = dcaplug.c\
extract_dca.c\
gettimeofday.c\
parse.c\
bitstream.c\
downmix.c\
convert2s16.c\
audio_out.h\
dca.h\
dts.h\
gettimeofday.h\
tendra.h\
dca_internal.h\
tables_adpcm.h\
tables_fir.h\
tables.h\
tables_huffman.h\
tables_quantization.h\
tables_vq.h

dca_la_LDFLAGS = -module

AM_CFLAGS = $(CFLAGS) -fPIC

endif
