if HAVE_CUE
pkglib_LTLIBRARIES = cue.la

cue_la_SOURCES = cue.c ../../shared/cueutil.c ../../shared/cueutil.h

cue_la_LDFLAGS = -module -avoid-version -lm

cue_la_LIBADD = $(LIBADD)

cue_la_CFLAGS = $(CFLAGS) -std=c99
endif
