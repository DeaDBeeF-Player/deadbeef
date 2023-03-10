if HAVE_MUSEPACK

pkglib_LTLIBRARIES = musepack.la

musepack_la_SOURCES = musepack.c\
huffman.c\
mpc_bits_reader.c\
mpc_decoder.c\
mpc_demux.c\
mpc_reader.c\
requant.c\
streaminfo.c\
synth_filter.c\
crc32.c\
decoder.h\
huffman.h\
internal.h\
mpc_bits_reader.h\
mpc/mpcdec.h\
mpcdec_math.h\
mpc/reader.h\
requant.h\
mpc/streaminfo.h\
mpc/mpc_types.h\
mpc/minimax.h

musepack_la_LDFLAGS = -module -avoid-version
musepack_la_LIBADD = $(LDADD) -lm

musepack_la_CFLAGS = $(CFLAGS) -fPIC -std=c99 -I@top_srcdir@/include

endif
