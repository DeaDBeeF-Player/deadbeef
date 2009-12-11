dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_CONFIG_HEADER(config.h)

PACKAGE="deadbeef"
VERSION="guiplug-snap"

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

PKG_CHECK_MODULES(DEPS, samplerate)
PKG_CHECK_MODULES(GTKUI_DEPS, gtk+-2.0 >= 2.12 gthread-2.0 glib-2.0, HAVE_GTK=1)
PKG_CHECK_MODULES(ALSA_DEPS, alsa, HAVE_ALSA=1)
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([dl], [main])

AX_CHECK_COMPILER_FLAGS(-msse2, SIMD_FLAGS="$SIMD_FLAGS -msse2";HAVE_SSE2=1, [])
if test ${HAVE_SSE2}; then
    AC_DEFINE(HAVE_SSE2,1,[Support SSE2 instructions])
fi
AC_SUBST(SIMD_FLAGS)

dnl curl lib
AC_CHECK_LIB([curl], [main], [HAVE_CURL=1])
AM_CONDITIONAL(HAVE_CURL, test $HAVE_CURL)
if test ${HAVE_CURL}; then
    CURL_LIBS="-lcurl"
    AC_SUBST(CURL_LIBS)
fi

dnl lastfm plugin
if test ${HAVE_CURL}; then
    LFM_DIR="plugins/lastfm"
    AC_SUBST(LFM_DIR)
fi

dnl mpgmad plugin
AC_CHECK_LIB([mad], [main], [HAVE_LIBMAD=1])
AM_CONDITIONAL(HAVE_LIBMAD, test $HAVE_LIBMAD)
if test ${HAVE_LIBMAD}; then
    MAD_LIBS="-lmad"
    MPGMAD_DIR="plugins/mpgmad"
    AC_SUBST(MAD_LIBS)
    AC_SUBST(MPGMAD_DIR)
fi

dnl vorbis plugin
AC_CHECK_LIB([vorbis], [main], [HAVE_VORBIS=1])
AC_CHECK_LIB([vorbisfile], [main], [HAVE_VORBISFILE=1])
AM_CONDITIONAL(HAVE_VORBIS, test $HAVE_VORBIS)
AM_CONDITIONAL(HAVE_VORBISFILE, test $HAVE_VORBISFILE)
if test ${HAVE_VORBIS} && test ${HAVE_VORBISFILE} ; then
    VORBIS_LIBS="-lvorbis -lvorbisfile"
    VORBIS_DIR="plugins/vorbis"
    AC_SUBST(VORBIS_DIR)
    AC_SUBST(VORBIS_LIBS)
fi

dnl flac plugin
AC_CHECK_LIB([FLAC], [main], [HAVE_FLAC=1])
AM_CONDITIONAL(HAVE_FLAC, test $HAVE_FLAC)
if test ${HAVE_FLAC} ; then
    FLAC_LIBS="-lFLAC"
    FLAC_DIR="plugins/flac"
    AC_SUBST(FLAC_LIBS)
    AC_SUBST(FLAC_DIR)
fi

dnl wavpack plugin
AC_CHECK_LIB([wavpack], [main], [HAVE_WAVPACK=1])
AM_CONDITIONAL(HAVE_WAVPACK, test $HAVE_WAVPACK)
if test ${HAVE_WAVPACK} ; then
    WAVPACK_LIBS="-lwavpack"
    WAVPACK_DIR="plugins/wavpack"
    AC_SUBST(WAVPACK_LIBS)
    AC_SUBST(WAVPACK_DIR)
fi

dnl libsndfile plugin
AC_CHECK_LIB([sndfile], [main], [HAVE_SNDFILE=1])
AM_CONDITIONAL(HAVE_SNDFILE, test $HAVE_SNDFILE)
if test ${HAVE_SNDFILE} ; then
    SNDFILE_LIBS="-lsndfile"
    SNDFILE_DIR="plugins/sndfile"
    AC_SUBST(SNDFILE_LIBS)
    AC_SUBST(SNDFILE_DIR)
fi

dnl vfs_curl plugin
if test ${HAVE_CURL}; then
    VFS_CURL_DIR="plugins/vfs_curl"
    AC_SUBST(VFS_CURL_DIR)
fi

dnl faad2 plugin
dnl AC_CHECK_LIB([faad], [main], [HAVE_FAAD=1])
dnl AC_CHECK_LIB([mp4ff], [main], [HAVE_MP4FF=1])
dnl AM_CONDITIONAL(HAVE_FAAD, test $HAVE_FAAD)
dnl AM_CONDITIONAL(HAVE_MP4FF, test $HAVE_MP4FF)
dnl if test ${HAVE_FAAD} && test ${HAVE_MP4FF} ; then
dnl     FAAD2_LIBS="-lfaad -lmp4ff -lmp4v2"
dnl     FAAD2_DIR="plugins/faad2"
dnl     AC_SUBST(FAAD2_LIBS)
dnl     AC_SUBST(FAAD2_DIR)
dnl fi

dnl cdda plugin
AC_CHECK_LIB([cdio], [main], [HAVE_CDIO=1])
AC_CHECK_LIB([cddb], [main], [HAVE_CDDB=1])
AM_CONDITIONAL(HAVE_CDIO, test $HAVE_CDIO)
AM_CONDITIONAL(HAVE_CDDB, test $HAVE_CDDB)
if test ${HAVE_CDIO} && test ${HAVE_CDDB}; then
    CDDA_LIBS="-lcdio -lcddb"
    CDDA_DIR="plugins/cdda"
    AC_SUBST(CDDA_LIBS)
    AC_SUBST(CDDA_DIR)
fi

dnl gtkui plugin
AM_CONDITIONAL(HAVE_GTK, test $HAVE_GTK)
if test ${HAVE_GTK}; then
    GTKUI_DIR="plugins/gtkui"
    AC_SUBST(GTKUI_DEPS_CFLAGS)
    AC_SUBST(GTKUI_DEPS_LIBS)
    AC_SUBST(GTKUI_DIR)
fi

AM_CONDITIONAL(HAVE_ALSA, test $HAVE_ALSA)
if test ${HAVE_ALSA}; then
    ALSA_DIR="plugins/alsa"
    AC_SUBST(ALSA_DEPS_CFLAGS)
    AC_SUBST(ALSA_DEPS_LIBS)
    AC_SUBST(ALSA_DIR)
fi

dnl print summary
echo
echo "plugin summary:"
echo

AC_DEFUN([PRINT_PLUGIN_INFO],
    [
        if $3 ; then
            echo "    $1: yes - $2"
        else
            echo "    $1: no - $2"
        fi
    ]
)

PRINT_PLUGIN_INFO([stdio],[Standard IO plugin],[true])
PRINT_PLUGIN_INFO([alsa],[alsa output],[test $HAVE_ALSA])
PRINT_PLUGIN_INFO([sid],[SID player based on libsidplay2],[true])
PRINT_PLUGIN_INFO([gme],[chiptune music player based on GME],[true])
PRINT_PLUGIN_INFO([dumb],[module player based on DUMB library],[true])
PRINT_PLUGIN_INFO([ffap],[Monkey's audio (APE) decoder],[true])
PRINT_PLUGIN_INFO([lastfm],[last.fm scrobbler],[test $HAVE_CURL])
PRINT_PLUGIN_INFO([mpgmad],[mpeg player based on libmad],[test $HAVE_LIBMAD])
PRINT_PLUGIN_INFO([vorbis],[ogg vorbis player],[test $HAVE_VORBISFILE && test $HAVE_VORBIS])
PRINT_PLUGIN_INFO([flac],[flac player],[test $HAVE_FLAC])
PRINT_PLUGIN_INFO([wavpack],[wavpack player],[test $HAVE_WAVPACK])
PRINT_PLUGIN_INFO([sndfile],[PCM (wav,aiff,etc) player based on libsndfile],[test $HAVE_SNDFILE])
PRINT_PLUGIN_INFO([vfs_curl],[http/ftp streaming support],[test $HAVE_CURL])
dnl PRINT_PLUGIN_INFO([faad2],[aac/mp4 player],[test $HAVE_FAAD && test $HAVE_MP4FF])
PRINT_PLUGIN_INFO([cdda],[cd audio player],[test $HAVE_CDIO && test $HAVE_CDDB])
PRINT_PLUGIN_INFO([gtkui],[GTK user interface],[test $HAVE_GTK])
echo

AC_OUTPUT([
Makefile
pixmaps/Makefile
icons/Makefile
gme/Game_Music_Emu-0.5.2/Makefile
gme/Game_Music_Emu-0.5.2/gme/Makefile
sid/sidplay-libs-2.1.0/Makefile
dumb/Makefile
plugins/alsa/Makefile
plugins/hotkeys/Makefile
plugins/lastfm/Makefile
plugins/ffap/Makefile
plugins/mpgmad/Makefile
plugins/vorbis/Makefile
plugins/flac/Makefile
plugins/wavpack/Makefile
plugins/sndfile/Makefile
plugins/vfs_curl/Makefile
plugins/cdda/Makefile
plugins/gtkui/Makefile
deadbeef.desktop
])

