if HAVE_DSP_SRC
pkglib_LTLIBRARIES = dsp_libsrc.la

dsp_libsrc_la_SOURCES = src.c src.h

dsp_libsrc_la_LDFLAGS = -module -avoid-version

dsp_libsrc_la_LIBADD = $(LIBADD) $(LIBSAMPLERATE_DEPS_LIBS)

dsp_libsrc_la_CFLAGS = $(CFLAGS) $(LIBSAMPLERATE_DEPS_CFLAGS) -std=c99 -I@top_srcdir@/include

endif
