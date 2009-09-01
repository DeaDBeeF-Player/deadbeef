demacdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = demac.la
demac_la_SOURCES = demac.c\
libdemac/crc.c\
libdemac/decoder.c\
libdemac/decoder.h\
libdemac/demac_config.h\
libdemac/demac.h\
libdemac/entropy.c\
libdemac/entropy.h\
libdemac/filter_1280_15.c\
libdemac/filter_16_11.c\
libdemac/filter_256_13.c\
libdemac/filter_32_10.c\
libdemac/filter_64_11.c\
libdemac/filter.h\
libdemac/parser.c\
libdemac/parser.h\
libdemac/predictor.c\
libdemac/predictor.h\
libdemac/vector_math16_armv5te.h\
libdemac/vector_math16_armv6.h\
libdemac/vector_math16_cf.h\
libdemac/vector_math32_armv4.h\
libdemac/vector_math_generic.h

EXTRA_demac_la_SOURCES = libdemac/filter.c

demac_la_LDFLAGS = -module
#demac_la_LIBADD = $(LDADD) $(demac_LIBS)
#AM_CFLAGS = $(demac_DEPS_CFLAGS) -std=c99



