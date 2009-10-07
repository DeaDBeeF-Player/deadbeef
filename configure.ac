dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_HEADER(config.h)

PACKAGE="deadbeef"
VERSION="0.2.3.2"

AM_INIT_AUTOMAKE($PACKAGE,$VERSION)

AC_PROG_CC
AC_PROG_CXX
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
LT_INIT
AC_CONFIG_MACRO_DIR([m4])
AC_C_BIGENDIAN

case "$host" in
  i386-*-* | i486-*-* | i586-*-* | i686-*-* | i86pc-*-*)
    AC_DEFINE(ARCH_X86_32, 1, [architecture is x86])
    ;;
  x86_64-*-*)
    AC_DEFINE(ARCH_X86_64, 1, [architecture is x86_64])
    ;;
  powerpc-*-* )
    AC_DEFINE(ARCH_PPC_32, 1, [architecture is ppc32])
    ;;
  powerpc64-*-* )
    AC_DEFINE(ARCH_PPC_64, 1, [architecture is ppc64])
    ;;
  *)
    AC_DEFINE(ARCH_UNKNOWN, 1, [architecture is unknown])
    ;;
esac

test "x$prefix" = xNONE && prefix=$ac_default_prefix

CFLAGS="$CFLAGS -D_GNU_SOURCE -DLIBDIR=\\\"$libdir\\\" -DPREFIX=\\\"$prefix\\\""
CPPFLAGS="$CFLAGS"

PKG_CHECK_MODULES(DEPS, gtk+-2.0 >= 2.12 gthread-2.0 glib-2.0 samplerate alsa)

AX_CHECK_COMPILER_FLAGS(-msse2, SIMD_FLAGS="$SIMD_FLAGS -msse2";HAVE_SSE2=1, [])
if test ${HAVE_SSE2}; then
    AC_DEFINE(HAVE_SSE2,1,[Support SSE2 instructions])
fi
AC_SUBST(SIMD_FLAGS)

dnl lastfm plugin
AC_CHECK_LIB([curl], [main], [HAVE_CURL=1])
if test ${HAVE_CURL}; then
    LFM_LIBS="-lcurl"
    LFM_DIR="plugins/lastfm"
    AC_SUBST(LFM_LIBS)
    AC_SUBST(LFM_DIR)
fi

dnl mpgmad plugin
AC_CHECK_LIB([mad], [main], [HAVE_LIBMAD=1])
if test ${HAVE_LIBMAD}; then
    MAD_LIBS="-lmad"
    MPGMAD_DIR="plugins/mpgmad"
    AC_SUBST(MAD_LIBS)
    AC_SUBST(MPGMAD_DIR)
fi

dnl vorbis plugin
AC_CHECK_LIB([vorbis], [main], [HAVE_VORBIS=1])
AC_CHECK_LIB([vorbisfile], [main], [HAVE_VORBISFILE=1])

if test ${HAVE_VORBIS} && test ${HAVE_VORBISFILE} ; then
    VORBIS_LIBS="-lvorbis -lvorbisfile"
    VORBIS_DIR="plugins/vorbis"
    AC_SUBST(VORBIS_DIR)
    AC_SUBST(VORBIS_LIBS)
fi

dnl flac plugin
AC_CHECK_LIB([FLAC], [main], [HAVE_FLAC=1])

if test ${HAVE_FLAC} ; then
    FLAC_LIBS="-lFLAC"
    FLAC_DIR="plugins/flac"
    AC_SUBST(FLAC_LIBS)
    AC_SUBST(FLAC_DIR)
fi

dnl wavpack plugin
AC_CHECK_LIB([wavpack], [main], [HAVE_WAVPACK=1])

if test ${HAVE_WAVPACK} ; then
    WAVPACK_LIBS="-lwavpack"
    WAVPACK_DIR="plugins/wavpack"
    AC_SUBST(WAVPACK_LIBS)
    AC_SUBST(WAVPACK_DIR)
fi

dnl libsndfile plugin
AC_CHECK_LIB([sndfile], [main], [HAVE_SNDFILE=1])

if test ${HAVE_SNDFILE} ; then
    SNDFILE_LIBS="-lsndfile"
    SNDFILE_DIR="plugins/sndfile"
    AC_SUBST(SNDFILE_LIBS)
    AC_SUBST(SNDFILE_DIR)
fi

AC_OUTPUT([
Makefile
pixmaps/Makefile
icons/Makefile
gme/Game_Music_Emu-0.5.2/Makefile
gme/Game_Music_Emu-0.5.2/gme/Makefile
sid/sidplay-libs-2.1.0/Makefile
dumb/Makefile
plugins/hotkeys/Makefile
plugins/lastfm/Makefile
plugins/ffap/Makefile
plugins/mpgmad/Makefile
plugins/vorbis/Makefile
plugins/flac/Makefile
plugins/wavpack/Makefile
plugins/sndfile/Makefile
deadbeef.desktop
])

