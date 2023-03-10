if HAVE_DCA

pkglib_LTLIBRARIES = dca.la

dca_la_SOURCES = dcaplug.c\
extract_dca.c\
gettimeofday.c\
parse.c\
bitstream.c\
downmix.c\
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
tables_vq.h\
bitstream.h

dca_la_LDFLAGS = -module -avoid-version
dca_la_LIBADD = $(LDADD) -lm

dca_la_CFLAGS = $(CFLAGS) -fPIC -std=c99 -I@top_srcdir@/include

endif
