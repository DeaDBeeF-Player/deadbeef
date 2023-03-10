plugindir = @top_srcdir@/plugins/ffap
EXTRA_DIST = $(plugindir)/x86inc.asm

if HAVE_FFAP
pkglib_LTLIBRARIES = ffap.la

topsrcdir = @top_srcdir@
YASM=$(topsrcdir)/yasmwrapper.sh
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

ffap_la_CFLAGS = $(CFLAGS) -fPIC -std=c99 -I@top_srcdir@/include
endif
