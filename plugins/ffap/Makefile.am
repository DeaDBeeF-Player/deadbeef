if HAVE_FFAP
ffapdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = ffap.la

topsrcdir = @top_srcdir@
EXTRA_PROGRAMS = foo
YASM=$(topsrcdir)/yasmwrapper.sh
SUFFIXES = .asm

if HAVE_YASM
INTEL_SRC=dsputil_yasm.asm
ffap_la_DEPENDENCIES=dsputil_yasm.lo
endif

ffap_la_SOURCES = ffap.c $(INTEL_SRC)

if HAVE_YASM
.asm.lo:
	$(LIBTOOL) --tag=CC --mode=compile $(YASM) $(YASM_FLAGS)  -o $@ $<
endif

ffap_la_LDFLAGS = -module

AM_CFLAGS = $(CFLAGS) -fPIC -std=c99
endif
