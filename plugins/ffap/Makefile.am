if HAVE_FFAP
ffapdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = ffap.la

topsrcdir = @top_srcdir@
YASM=$(topsrcdir)/yasmwrapper.sh
EXTRA_DIST = dsputil_yasm.asm x86inc.asm
SUFFIXES = .asm

if HAVE_YASM
if APE_USE_YASM
INTEL_SRC=dsputil_yasm.asm
ffap_la_DEPENDENCIES=dsputil_yasm.lo
endif
endif

ffap_la_SOURCES = ffap.c $(INTEL_SRC)

if HAVE_YASM
if APE_USE_YASM
.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile $(YASM) $(YASM_FLAGS)  -o $@ $<
endif
endif

ffap_la_LDFLAGS = -module -avoid-version -lm

AM_CFLAGS = $(CFLAGS) -fPIC -std=c99
endif
