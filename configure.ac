dnl Process this file with autoconf to produce a configure script.
AC_INIT([deadbeef], [devel])

AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([1.11 dist-bzip2])
AM_MAINTAINER_MODE()

AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_CXX
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_C_BIGENDIAN
AM_GNU_GETTEXT

IT_PROG_INTLTOOL([0.41.0])
GETTEXT_PACKAGE=deadbeef
AC_SUBST(GETTEXT_PACKAGE)

test "$prefix" = NONE && prefix=/usr/local
AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Name of gettext locale directory])

if test "x$USE_MAINTAINER_MODE" = "xyes" ; then
    AM_PROG_VALAC([0.7.0])
    if test "x$VALAC" = "x" ; then
        AC_MSG_ERROR([Cannot find the "valac" compiler in your PATH])
    fi
fi

case "$host" in
  i386-*-* | i486-*-* | i586-*-* | i686-*-* | i86pc-*-*)
    AC_DEFINE(ARCH_X86_32, 1, [architecture is x86])
    ;;
  x86_64-*-* | amd64-*-*)
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

dnl INSANE_CFLAGS="-Wformat -Wdisabled-optimization -Wcomment -Wchar-subscripts -Wunused-function -Wunused-value -Wuninitialized -Wtype-limits -Wbad-function-cast"
dnl INSANE_CXXFLAGS="-Wcomment -Wchar-subscripts -Wunused-function -Wunused-value -Wuninitialized -Wtype-limits"

AC_SUBST(INSANE_CFLAGS)
AC_SUBST(INSANE_CXXFLAGS)

CXXFLAGS="$CXXFLAGS $INSANE_CXXFLAGS -D_GNU_SOURCE -DLIBDIR=\\\"$libdir\\\" -DPREFIX=\\\"$prefix\\\" -DDOCDIR=\\\"$docdir\\\""
CFLAGS="$CFLAGS $INSANE_CFLAGS -D_GNU_SOURCE -DLIBDIR=\\\"$libdir\\\" -DPREFIX=\\\"$prefix\\\" -DDOCDIR=\\\"$docdir\\\""

AC_ARG_ENABLE(nullout,  [  --disable-nullout       disable NULL output plugin (default: enabled)], [enable_nullout=$enableval], [enable_nullout=yes])
AC_ARG_ENABLE(alsa,     [  --disable-alsa          disable ALSA output plugin (default: enabled)], [enable_alsa=$enableval], [enable_alsa=yes])
AC_ARG_ENABLE(oss,      [  --disable-oss           disable Open Sound System output plugin (default: enabled)], [enable_oss=$enableval], [enable_oss=yes])
AC_ARG_ENABLE(pulse,    [  --disable-pulse         disable PulseAudio output plugin (default: enabled)], [enable_pulse=$enableval], [enable_pulse=yes])
AC_ARG_ENABLE(gtkui,    [  --disable-gtkui         disable standard GTK2 user interface plugin (default: enabled)], [enable_gtkui=$enableval], [enable_gtkui=yes])
AC_ARG_ENABLE(gtk3,     [  --enable-gtk3           use GTK3 library (default: disabled)], [enable_gtk3=$enableval], [enable_gtk3=no])
AC_ARG_ENABLE(vfs_curl, [  --disable-vfs-curl      disable HTTP streaming vfs plugin (default: enabled)], [enable_vfs_curl=$enableval], [enable_vfs_curl=yes])
AC_ARG_ENABLE(lfm,      [  --disable-lfm           disable last.fm/libre.fm scrobbler plugin (default: enabled)], [enable_lfm=$enableval], [enable_lfm=yes])
AC_ARG_ENABLE(artwork,  [  --disable-artwork       disable album art loader plugin (default: enabled)], [enable_artwork=$enableval], [enable_artwork=yes])
AC_ARG_ENABLE(supereq,  [  --disable-supereq       disable SuperEQ DSP plugin (default: enabled)], [enable_supereq=$enableval], [enable_supereq=yes])
AC_ARG_ENABLE(sid,      [  --disable-sid           disable commodore64 SID music player plugin (default: enabled)], [enable_sid=$enableval], [enable_sid=yes])
AC_ARG_ENABLE(mad,      [  --disable-mpgmad        disable mpeg (mad) plugin (default: enabled)], [enable_mpgmad=$enableval], [enable_mpgmad=yes])
AC_ARG_ENABLE(ffap,     [  --disable-ffap          disable Monkey's Audio plugin (default: enabled)], [enable_ffap=$enableval], [enable_ffap=yes])
AC_ARG_ENABLE(vtx,      [  --disable-vtx           disable libayemy VTX ZX-Spectrum music player plugin (default: enabled)], [enable_vtx=$enableval], [enable_vtx=yes])
AC_ARG_ENABLE(adplug,   [  --disable-adplug        disable adplug plugin (default: enabled)], [enable_adplug=$enableval], [enable_adplug=yes])
AC_ARG_ENABLE(hotkeys,  [  --disable-hotkeys       disable global hotkeys plugin (default: enabled)], [enable_hotkeys=$enableval], [enable_hotkeys=yes])
AC_ARG_ENABLE(vorbis,   [  --disable-vorbis        disable Ogg Vorbis player plugin (default: enabled)], [enable_vorbis=$enableval], [enable_vorbis=yes])
AC_ARG_ENABLE(ffmpeg,   [  --disable-ffmpeg        disable FFMPEG plugin for WMA, MPC, TTA, etc (default: enabled)], [enable_ffmpeg=$enableval], [enable_ffmpeg=yes])
AC_ARG_ENABLE(flac,     [  --disable-flac          disable FLAC player plugin (default: enabled)], [enable_flac=$enableval], [enable_flac=yes])
AC_ARG_ENABLE(sndfile,  [  --disable-sndfile       disable libsndfile plugin for PCM wave files (default: enabled)], [enable_sndfile=$enableval], [enable_sndfile=yes])
AC_ARG_ENABLE(wavpack,  [  --disable-wavpack       disable wavpack plugin (default: enabled)], [enable_wavpack=$enableval], [enable_wavpack=yes])
AC_ARG_ENABLE(cdda,     [  --disable-cdda          disable CD-Audio plugin (default: enabled)], [enable_cdda=$enableval], [enable_cdda=yes])
AC_ARG_ENABLE(gme,      [  --disable-gme           disable Game Music Emu plugin for NSF, AY, etc (default: enabled)], [enable_gme=$enableval], [enable_gme=yes])
AC_ARG_ENABLE(dumb,     [  --disable-dumb          disable D.U.M.B. plugin for MOD, S3M and other tracker formats (default: enabled)], [enable_dumb=$enableval], [enable_dumb=yes])
AC_ARG_ENABLE(notify,   [  --disable-notify        disable notification-daemon support plugin (default: enabled)], [enable_notify=$enableval], [enable_notify=yes])

PKG_CHECK_MODULES(DEPS, samplerate)

if test "x$enable_gtkui" != "xno" ; then
    if test "x$enable_gtk3" == "xyes" ; then
        PKG_CHECK_MODULES(GTKUI_DEPS, gtk+-3.0 >= 2.90 gthread-2.0 glib-2.0, HAVE_GTK=yes, HAVE_GTK=no)
    else
        PKG_CHECK_MODULES(GTKUI_DEPS, gtk+-2.0 >= 2.12 gthread-2.0 glib-2.0, HAVE_GTK=yes, HAVE_GTK=no)
    fi
else
    HAVE_GTK=no
fi

if test "x$enable_alsa" != "xno" ; then
    PKG_CHECK_MODULES(ALSA_DEPS, alsa, HAVE_ALSA=yes, HAVE_ALSA=no)
fi

if test "x$enable_ffmpeg" != "xno" ; then
    PKG_CHECK_MODULES(FFMPEG_DEPS, libavcodec >= 51.0.0 libavutil libavformat >= 52.0.0, HAVE_FFMPEG=yes, HAVE_FFMPEG=no)
fi

if test "x$enable_pulse" != "xno" ; then
    PKG_CHECK_MODULES(PULSE_DEPS, libpulse-simple, HAVE_PULSE=yes, HAVE_PULSE=no)
fi

AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([dl], [main])

AC_CHECK_HEADER([iconv.h],[],[iconv.h not found.])

AC_CHECK_LIB([iconv], [main], [have_iconv=yes], [have_iconv=no])
if test "x${have_iconv}" = "xyes" ; then
ICONV_LIB="-liconv"
    AC_SUBST(ICONV_LIB)
    AC_DEFINE(HAVE_LIBICONV,1,[Use libiconv instead of glibc iconv])
fi

AX_CHECK_COMPILER_FLAGS(-msse2, HAVE_SSE2=1, [])
if test ${HAVE_SSE2}; then
    AC_DEFINE(HAVE_SSE2,1,[Support SSE2 instructions])
fi

dnl curl lib
AC_CHECK_LIB([curl], [main], [HAVE_CURL=yes])
if test "x$HAVE_CURL" = "xyes"; then
    CURL_LIBS="-lcurl"
    AC_SUBST(CURL_LIBS)
fi

PKG_CHECK_MODULES(DBUS_DEPS, dbus-1, HAVE_DBUS=yes, HAVE_DBUS=no)

dnl mpgmad plugin

if test "x$enable_mpgmad" != "xno" ; then
    AC_CHECK_LIB([mad], [main], [HAVE_LIBMAD=yes])
    if test "x$HAVE_LIBMAD" = "xyes" ; then
        HAVE_MPGMAD=yes
        MAD_LIBS="-lmad"
        AC_SUBST(MAD_LIBS)
    fi
fi

dnl vorbis plugin
if test "x$enable_vorbis" != "xno" ; then
    AC_CHECK_LIB([vorbis], [main], [HAVE_VORBIS=yes])
    AC_CHECK_LIB([vorbisfile], [main], [HAVE_VORBISFILE=yes])
    if test "x$HAVE_VORBIS" = "xyes" && test "x$HAVE_VORBISFILE" = "xyes" ; then
        HAVE_VORBISPLUGIN=yes
        VORBIS_LIBS="-lvorbis -lvorbisfile"
        AC_SUBST(VORBIS_LIBS)
    fi
fi

dnl flac plugin
if test "x$enable_flac" != "xno" ; then
    AC_CHECK_LIB([FLAC], [main], [HAVE_FLAC=yes])
    if test "x$HAVE_FLAC" = "xyes" ; then
        HAVE_FLACPLUGIN=yes
        FLAC_LIBS="-lFLAC"
        AC_SUBST(FLAC_LIBS)
    fi
fi

dnl wavpack plugin
if test "x$enable_wavpack" != "xno" ; then
    AC_CHECK_LIB([wavpack], [main], [HAVE_WAVPACK=yes])
    if test "x$HAVE_WAVPACK" = "xyes" ; then
        HAVE_WAVPACKPLUGIN=yes
        WAVPACK_LIBS="-lwavpack"
        AC_SUBST(WAVPACK_LIBS)
    fi
fi

dnl libsndfile plugin
if test "x$enable_sndfile" != "xno" ; then
    AC_CHECK_LIB([sndfile], [main], [HAVE_SNDFILE=yes])
    if test "x$HAVE_SNDFILE" = "xyes" ; then
        HAVE_SNDFILEPLUGIN=yes
        SNDFILE_LIBS="-lsndfile"
        AC_SUBST(SNDFILE_LIBS)
    fi
fi

dnl vfs_curl plugin
if test "x$enable_vfs_curl" != "xno" ; then
    if test "x$HAVE_CURL" = "xyes" ; then
        HAVE_VFS_CURL=yes
        VFS_CURL_LIBS="-lcurl"
        AC_SUBST(VFS_CURL_LIBS)
    fi
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
if test "x$enable_cdda" != "xno" ; then
    AC_CHECK_LIB([cdio], [main], [HAVE_CDIO=yes])
    AC_CHECK_LIB([cddb], [main], [HAVE_CDDB=yes])
    if test "x$HAVE_CDIO" = "xyes" && test "x$HAVE_CDDB" = "xyes" ; then
        HAVE_CDDAPLUGIN=yes
        CDDA_LIBS="-lcdio -lcddb"
        AC_SUBST(CDDA_LIBS)
    fi
fi

dnl gtkui plugin
if test "x$enable_gtkui" != "xno" ; then
    if test "x$HAVE_GTK" = "xyes" ; then
        HAVE_GTKUI=yes
    fi
fi

if test "x$enable_alsa" != "xno" ; then
    if test "x$HAVE_ALSA" = "xyes" ; then
        HAVE_ALSAPLUGIN=yes
    fi
fi

if test "x$enable_ffmpeg" != "xno" ; then
    if test "x$HAVE_FFMPEG" = "xyes" ; then
        HAVE_FFMPEGPLUGIN=yes
        AC_CHECK_HEADER([ffmpeg/avformat.h], FFMPEG_DEPS_CFLAGS="$FFMPEG_DEPS_CFLAGS -D FFMPEG_OLD")
    fi
fi

if test "x$enable_hotkeys" != "xno" ; then
    AC_CHECK_HEADER([X11/Xlib.h], HAVE_XLIB_H=yes)
    if test "x$HAVE_XLIB_H" = "xyes" ; then
        HAVE_HOTKEYS=yes
        HOTKEYS_LIBS="-lX11"
        AC_SUBST(HOTKEYS_LIBS)
    fi
fi

dnl *** OSS output (partly stolen from audacious)
if test "x$enable_oss" != "xno"; then
    have_oss=no

    AC_MSG_CHECKING(for OSS include dir)
    OSS_CFLAGS=""
    if test -f "/etc/oss.conf" ; then
        for i in `cat /etc/oss.conf`; do
            t=`echo $i | sed -e 's/OSSLIBDIR=//'`
            if test "x$i" != "x$t" ; then
                if test -f "$t/include/sys/soundcard.h" -o -f "$i/include/soundcard.h" ; then
                    OSS_CFLAGS="-I$t/include/sys"
                    AC_MSG_RESULT([$OSS_CFLAGS])
                    have_oss=yes
                fi
            fi
        done
    else
        AC_MSG_RESULT([not found])
    fi

    if test "x$have_oss" != "xyes"; then
        AC_MSG_CHECKING(for sys/soundcard.h)
        AC_CHECK_HEADERS(sys/soundcard.h)
        if test "x${ac_cv_header_sys_soundcard_h}" = "xyes" ; then
            have_oss=yes
            AC_MSG_RESULT([found])
        else
            AC_MSG_RESULT([not found])
        fi
    fi


    if test "x$have_oss" = "xyes"; then
        HAVE_OSS=yes
        AC_SUBST(OSS_CFLAGS)
    fi
fi

if test "x$enable_pulseaudio" != "xno"; then
    if test "x$HAVE_PULSE" = "xyes"; then
        HAVE_PULSEPLUGIN=yes
    fi
fi

dnl lastfm plugin
if test "x$enable_lfm" != "xno" ; then
    if test "x$HAVE_CURL" = "xyes" && test "x$enable_lastfm" != "xno" ; then
        HAVE_LASTFM=yes
    fi
fi

if test "x$enable_artwork" != "xno" ; then
    if test "x$HAVE_CURL" = "xyes" && test "x$HAVE_VFS_CURL" = "xyes" ; then
        HAVE_ARTWORK=yes
    fi
fi

if test "x$enable_adplug" != "xno" ; then
    HAVE_ADPLUG=yes
fi

if test "x$enable_ffap" != "xno" ; then
    HAVE_FFAP=yes
fi

if test "x$enable_sid" != "xno" ; then
    HAVE_SID=yes
fi

if test "x$enable_nullout" != "xno" ; then
    HAVE_NULLOUT=yes
fi

if test "x$enable_supereq" != "xno" ; then
    HAVE_SUPEREQ=yes
fi

if test "x$enable_vtx" != "xno" ; then
    HAVE_VTX=yes
fi

if test "x$enable_gme" != "xno" ; then
    HAVE_GME=yes
fi

if test "x$enable_dumb" != "xno" ; then
    HAVE_DUMB=yes
fi

if test "x$HAVE_DBUS" = "xyes" && test "x$enable_notify" != "xno" ; then
    HAVE_NOTIFY=yes
    NOTIFY_LIBS="$DBUS_DEPS_LIBS"
    NOTIFY_CFLAGS="$DBUS_DEPS_CFLAGS"
    AC_SUBST(NOTIFY_LIBS)
    AC_SUBST(NOTIFY_CFLAGS)
fi

PLUGINS_DIRS="plugins/lastfm plugins/mpgmad plugins/vorbis plugins/flac plugins/wavpack plugins/sndfile plugins/vfs_curl plugins/cdda plugins/gtkui plugins/alsa plugins/ffmpeg plugins/hotkeys plugins/oss plugins/artwork plugins/adplug plugins/ffap plugins/sid plugins/nullout plugins/supereq plugins/vtx plugins/gme plugins/dumb plugins/pulse plugins/notify"

AM_CONDITIONAL(HAVE_VORBIS, test "x$HAVE_VORBISPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_FLAC, test "x$HAVE_FLACPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_WAVPACK, test "x$HAVE_WAVPACKPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_SNDFILE, test "x$HAVE_SNDFILEPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_CDDA, test "x$HAVE_CDDAPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_GTKUI, test "x$HAVE_GTKUI" = "xyes")
AM_CONDITIONAL(HAVE_ALSA, test "x$HAVE_ALSAPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_FFMPEG, test "x$HAVE_FFMPEGPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_PULSE, test "x$HAVE_PULSEPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_ARTWORK, test "x$HAVE_ARTWORK" = "xyes")
AM_CONDITIONAL(HAVE_ADPLUG, test "x$HAVE_ADPLUG" = "xyes")
AM_CONDITIONAL(HAVE_FFAP, test "x$HAVE_FFAP" = "xyes")
AM_CONDITIONAL(HAVE_SID, test "x$HAVE_SID" = "xyes")
AM_CONDITIONAL(HAVE_NULLOUT, test "x$HAVE_NULLOUT" = "xyes")
AM_CONDITIONAL(HAVE_VTX, test "x$HAVE_VTX" = "xyes")
AM_CONDITIONAL(HAVE_GME, test "x$HAVE_GME" = "xyes")
AM_CONDITIONAL(HAVE_DUMB, test "x$HAVE_DUMB" = "xyes")
AM_CONDITIONAL(HAVE_LASTFM, test "x$HAVE_LASTFM" = "xyes")
AM_CONDITIONAL(HAVE_VFS_CURL, test "x$HAVE_VFS_CURL" = "xyes")
AM_CONDITIONAL(HAVE_HOTKEYS, test "x$HAVE_HOTKEYS" = "xyes")
AM_CONDITIONAL(HAVE_MPGMAD, test "x$HAVE_MPGMAD" = "xyes")
AM_CONDITIONAL(HAVE_OSS, test "x$HAVE_OSS" = "xyes")
AM_CONDITIONAL(HAVE_SUPEREQ, test "x$HAVE_SUPEREQ" = "xyes")
AM_CONDITIONAL(HAVE_NOTIFY, test "x$HAVE_NOTIFY" = "xyes")

AC_SUBST(PLUGINS_DIRS)

if test "x$USE_MAINTAINER_MODE" = "xyes" ; then
echo
echo "Build Configuration:"
echo
echo "* Maintainer Mode:        $USE_MAINTAINER_MODE"
echo "    * Vala:               $VALAC"
fi

dnl print summary
echo
echo "Plugin Summary:"
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
PRINT_PLUGIN_INFO([gme],[chiptune music player based on GME],[test "x$HAVE_GME" = "xyes"])
PRINT_PLUGIN_INFO([dumb],[module player based on DUMB library],[test "x$HAVE_DUMB" = "xyes"])
PRINT_PLUGIN_INFO([nullout],[NULL output],[test "x$HAVE_NULLOUT" = "xyes"])
PRINT_PLUGIN_INFO([alsa],[ALSA output],[test "x$HAVE_ALSA" = "xyes"])
PRINT_PLUGIN_INFO([sid],[SID player based on libsidplay2],[test "x$HAVE_SID" = "xyes"])
PRINT_PLUGIN_INFO([ffap],[Monkey's audio (APE) decoder],[test "x$HAVE_FFAP" = "xyes"])
PRINT_PLUGIN_INFO([lastfm],[last.fm scrobbler],[test "x$HAVE_LASTFM" = "xyes"])
PRINT_PLUGIN_INFO([mpgmad],[mpeg player based on libmad],[test "x$HAVE_MPGMAD" = "xyes"])
PRINT_PLUGIN_INFO([vorbis],[ogg vorbis player],[test "x$HAVE_VORBISPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([flac],[flac player],[test "x$HAVE_FLACPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([wavpack],[wavpack player],[test "x$HAVE_WAVPACKPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([sndfile],[PCM (wav,aiff,etc) player based on libsndfile],[test "x$HAVE_SNDFILEPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([vtx],[vtx file player (ay8910/12 emulation)],[test "x$HAVE_VTX" = "xyes"])
PRINT_PLUGIN_INFO([adplug],[adplug player (OPL2/OPL3 emulation)],[test "x$HAVE_ADPLUG" = "xyes"])
PRINT_PLUGIN_INFO([vfs_curl],[http/ftp streaming support],[test "x$HAVE_VFS_CURL" = "xyes"])
dnl PRINT_PLUGIN_INFO([faad2],[aac/mp4 player],[test $HAVE_FAAD && test $HAVE_MP4FF])
PRINT_PLUGIN_INFO([cdda],[cd audio player],[test "x$HAVE_CDDAPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([gtkui],[GTK user interface],[test "x$HAVE_GTK" = "xyes"])
PRINT_PLUGIN_INFO([hotkeys],[Global hotkeys support],[test "x$HAVE_HOTKEYS" = "xyes"])
PRINT_PLUGIN_INFO([ffmpeg],[ffmpeg codecs],[test "x$HAVE_FFMPEGPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([oss],[oss output plugin],[test "x$HAVE_OSS" = "xyes"])
PRINT_PLUGIN_INFO([pulse],[PulseAudio output plugin],[test "x$HAVE_PULSEPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([artwork],[Cover art plugin],[test "x$HAVE_ARTWORK" = "xyes"])
PRINT_PLUGIN_INFO([supereq],[Equalizer based on Super EQ library by Naoki Shibata],[test "x$HAVE_SUPEREQ" = "xyes"])
PRINT_PLUGIN_INFO([notify],[notification-daemon support plugin],[test "x$HAVE_NOTIFY" = "xyes"])

echo

AC_CONFIG_FILES([
Makefile
pixmaps/Makefile
icons/Makefile
plugins/gme/Makefile
plugins/dumb/Makefile
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
plugins/nullout/Makefile
plugins/vtx/Makefile
plugins/adplug/Makefile
plugins/ffmpeg/Makefile
plugins/sid/Makefile
plugins/oss/Makefile
plugins/pulse/Makefile
plugins/artwork/Makefile
plugins/supereq/Makefile
plugins/notify/Makefile
intl/Makefile
po/Makefile.in
deadbeef.desktop
])

AC_OUTPUT
