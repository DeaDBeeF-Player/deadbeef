if HAVE_SUPEREQ
supereqdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = supereq.la
supereq_la_SOURCES = supereq.c supereq.h Equ.cpp Fftsg_fl.cpp paramlist.hpp\
ffmpeg_fft/libavutil/mem.c\
ffmpeg_fft/libavutil/mathematics.c\
ffmpeg_fft/libavutil/rational.c\
ffmpeg_fft/libavutil/intfloat_readwrite.c\
ffmpeg_fft/libavcodec/dct.c\
ffmpeg_fft/libavcodec/avfft.c\
ffmpeg_fft/libavcodec/fft.c\
ffmpeg_fft/libavcodec/dct32.c\
ffmpeg_fft/libavcodec/rdft.c\
ffmpeg_fft/libavutil/intfloat_readwrite.h\
ffmpeg_fft/libavutil/avutil.h\
ffmpeg_fft/libavutil/common.h\
ffmpeg_fft/libavutil/attributes.h\
ffmpeg_fft/libavutil/mem.h\
ffmpeg_fft/libavutil/avconfig.h\
ffmpeg_fft/libavutil/mathematics.h\
ffmpeg_fft/libavutil/rational.h\
ffmpeg_fft/publik.h\
ffmpeg_fft/ffmpeg_fft.h\
ffmpeg_fft/libavcodec/dct32.h\
ffmpeg_fft/libavcodec/fft.h\
ffmpeg_fft/libavcodec/avfft.h\
ffmpeg_fft/config.h\
ff_rdft.c

AM_CFLAGS = $(CFLAGS) -I ffmpeg_fft -I ffmpeg_fft/libavcodec -I ffmpeg_fft/libavutil -std=c99
AM_CPPFLAGS = $(CXXFLAGS) -fno-exceptions -fno-rtti -nostdlib -fno-unwind-tables -I ffmpeg_fft -I ffmpeg_fft/libavcodec -I ffmpeg_fft/libavutil

supereq_la_LDFLAGS = -module -nostdlib -lsupc++

supereq_la_LIBADD = $(LDADD)
endif
