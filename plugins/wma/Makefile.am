if HAVE_WMA
wmapath=@top_srcdir@/plugins/wma
pkglib_LTLIBRARIES = wma.la
wma_la_SOURCES = wma_plugin.c\
	asfheader.c\
	libasf/asf.c\
	libasf/asf.h\
	libwma/wmafixed.c\
	libwma/wmadeci.c\
	libwma/fft-ffmpeg.c\
	libwma/mdct_lookup.c\
	libwma/mdct.c\
	libwma/ffmpeg_bitstream.c\
	libwma/asm_arm.h\
	libwma/fft-ffmpeg_arm.h\
	libwma/types.h\
	libwma/codeclib_misc.h\
	libwma/fft.h\
	libwma/wmadata.h\
	libwma/ffmpeg_get_bits.h\
	libwma/mdct.h\
	libwma/wmadec.h\
	libwma/ffmpeg_intreadwrite.h\
	libwma/mdct_lookup.h\
	libwma/wmafixed.h

#	libwma-ff/wma.c\
#	libwma-ff/wmadec.c\
#	libwma-ff/wma_common.c\
#	libwma-ff/bitstream.c\
#	libwma-ff/mathematics.c\
#	libwma-ff/mdct.c\
#	libwma-ff/fft.c\
#	libwma-ff/aactab.c




wma_la_LDFLAGS = -module -avoid-version

wma_la_LIBADD = $(LDADD)
wma_la_CFLAGS = $(CFLAGS) -std=c99 -I$(wmapath) -I@top_srcdir@/include
endif


