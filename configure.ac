dnl Process this file with autoconf to produce a configure script.
AC_INIT([deadbeef], [devel])

AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([1.11 dist-bzip2])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE()

AC_USE_SYSTEM_EXTENSIONS
LT_INIT
AC_PROG_CC
AC_PROG_CXX
AC_STDC_HEADERS
AC_PROG_INSTALL
#AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_C_BIGENDIAN
AM_GNU_GETTEXT
PKG_PROG_PKG_CONFIG

IT_PROG_INTLTOOL([0.40.0])
GETTEXT_PACKAGE=deadbeef
AC_SUBST(GETTEXT_PACKAGE)

test "$prefix" = NONE && prefix=/usr/local
AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Name of gettext locale directory])

case "$host" in
  i386-*-* | i486-*-* | i586-*-* | i686-*-* | i86pc-*-*)
    AC_DEFINE(ARCH_X86_32, 1, [architecture is x86])
    LIB="../deadbeef-deps/lib-x86-32"
    YASM_FLAGS="-f elf -D ARCH_X86_32 -m x86"
    APE_USE_YASM=yes
    ;;
  x86_64-apple-*)
    AC_DEFINE(ARCH_X86_64_OSX, 1, [architecture is x86_64 for OSX])
    LIB="../deadbeef-deps/lib-x86-64-osx"
    ;;
  x86_64-*-* | amd64-*-*)
    AC_DEFINE(ARCH_X86_64, 1, [architecture is x86_64])
    LIB="../deadbeef-deps/lib-x86-64"
    YASM_FLAGS="-f elf -D ARCH_X86_64 -m amd64"
    APE_USE_YASM=yes
    ;;
  powerpc-*-* )
    AC_DEFINE(ARCH_PPC_32, 1, [architecture is ppc32])
    LIB="../deadbeef-deps/lib-ppc-32"
    ;;
  powerpc64-*-* )
    AC_DEFINE(ARCH_PPC_64, 1, [architecture is ppc64])
    LIB="../deadbeef-deps/lib-ppc-64"
    ;;
  *)
    AC_DEFINE(ARCH_UNKNOWN, 1, [architecture is unknown])
    ;;
esac
AC_SUBST(LIB)
AC_SUBST(YASM_FLAGS)

test "x$prefix" = xNONE && prefix=$ac_default_prefix

dnl INSANE_CFLAGS="-Wformat -Wdisabled-optimization -Wcomment -Wchar-subscripts -Wunused-function -Wunused-value -Wuninitialized -Wtype-limits -Wbad-function-cast"
dnl INSANE_CXXFLAGS="-Wcomment -Wchar-subscripts -Wunused-function -Wunused-value -Wuninitialized -Wtype-limits"

AC_SUBST(INSANE_CFLAGS)
AC_SUBST(INSANE_CXXFLAGS)

AC_ARG_ENABLE(nullout,  [AS_HELP_STRING([--disable-nullout ], [disable NULL output plugin (default: enabled)])], [enable_nullout=$enableval], [enable_nullout=yes])
AC_ARG_ENABLE(alsa,     [AS_HELP_STRING([--disable-alsa    ], [disable ALSA output plugin (default: enabled)])], [enable_alsa=$enableval], [enable_alsa=yes])
AC_ARG_ENABLE(oss,      [AS_HELP_STRING([--disable-oss     ], [disable Open Sound System output plugin (default: enabled)])], [enable_oss=$enableval], [enable_oss=yes])
AC_ARG_ENABLE(pulse,    [AS_HELP_STRING([--disable-pulse   ], [disable PulseAudio output plugin (default: enabled)])], [enable_pulse=$enableval], [enable_pulse=yes])
AC_ARG_ENABLE(gtkui,    [AS_HELP_STRING([--disable-gtkui   ], [disable standard GTK+ user interface plugin (default: enabled)])], [enable_gtkui=$enableval], [enable_gtkui=yes])
AC_ARG_ENABLE(gtk3,     [AS_HELP_STRING([--enable-gtk3     ], [build GTK3 version of gtkui plugin (default: disabled)])], [enable_gtk3=$enableval], [enable_gtk3=no])
AC_ARG_ENABLE(gtk2,     [AS_HELP_STRING([--disable-gtk2     ], [build GTK2 version of gtkui plugin (default: enabled)])], [enable_gtk2=$enableval], [enable_gtk2=yes])
AC_ARG_ENABLE(vfs_curl, [AS_HELP_STRING([--disable-vfs-curl], [disable HTTP streaming vfs plugin (default: enabled)])], [enable_vfs_curl=$enableval], [enable_vfs_curl=yes])
AC_ARG_ENABLE(lfm,      [AS_HELP_STRING([--disable-lfm     ], [disable last.fm/libre.fm scrobbler plugin (default: enabled)])], [enable_lfm=$enableval], [enable_lfm=yes])
AC_ARG_ENABLE(artwork,  [AS_HELP_STRING([--disable-artwork ], [disable album art loader plugin (default: enabled)])], [enable_artwork=$enableval], [enable_artwork=yes])
AC_ARG_ENABLE(supereq,  [AS_HELP_STRING([--disable-supereq ], [disable SuperEQ DSP plugin (default: enabled)])], [enable_supereq=$enableval], [enable_supereq=yes])
AC_ARG_ENABLE(sid,      [AS_HELP_STRING([--disable-sid     ], [disable commodore64 SID music player plugin (default: enabled)])], [enable_sid=$enableval], [enable_sid=yes])
AC_ARG_ENABLE(mad,      [AS_HELP_STRING([--disable-mad     ], [disable mpeg (mad) plugin (default: enabled)])], [enable_mpgmad=$enableval], [enable_mpgmad=yes])
AC_ARG_ENABLE(ffap,     [AS_HELP_STRING([--disable-ffap    ], [disable Monkey's Audio plugin (default: enabled)])], [enable_ffap=$enableval], [enable_ffap=yes])
AC_ARG_ENABLE(vtx,      [AS_HELP_STRING([--disable-vtx     ], [disable libayemy VTX ZX-Spectrum music player plugin (default: enabled)])], [enable_vtx=$enableval], [enable_vtx=yes])
AC_ARG_ENABLE(adplug,   [AS_HELP_STRING([--disable-adplug  ], [disable adplug plugin (default: enabled)])], [enable_adplug=$enableval], [enable_adplug=yes])
AC_ARG_ENABLE(hotkeys,  [AS_HELP_STRING([--disable-hotkeys ], [disable global hotkeys plugin (default: enabled)])], [enable_hotkeys=$enableval], [enable_hotkeys=yes])
AC_ARG_ENABLE(vorbis,   [AS_HELP_STRING([--disable-vorbis  ], [disable Ogg Vorbis player plugin (default: enabled)])], [enable_vorbis=$enableval], [enable_vorbis=yes])
AC_ARG_ENABLE(ffmpeg,   [AS_HELP_STRING([--disable-ffmpeg  ], [disable FFMPEG plugin for WMA, MPC, TTA, etc (default: enabled)])], [enable_ffmpeg=$enableval], [enable_ffmpeg=yes])
AC_ARG_ENABLE(flac,     [AS_HELP_STRING([--disable-flac    ], [disable FLAC player plugin (default: enabled)])], [enable_flac=$enableval], [enable_flac=yes])
AC_ARG_ENABLE(sndfile,  [AS_HELP_STRING([--disable-sndfile ], [disable libsndfile plugin for PCM wave files (default: enabled)])], [enable_sndfile=$enableval], [enable_sndfile=yes])
AC_ARG_ENABLE(wavpack,  [AS_HELP_STRING([--disable-wavpack ], [disable wavpack plugin (default: enabled)])], [enable_wavpack=$enableval], [enable_wavpack=yes])
AC_ARG_ENABLE(cdda,     [AS_HELP_STRING([--disable-cdda    ], [disable CD-Audio plugin (default: enabled)])], [enable_cdda=$enableval], [enable_cdda=yes])
AC_ARG_ENABLE(gme,      [AS_HELP_STRING([--disable-gme     ], [disable Game Music Emu plugin for NSF, AY, etc (default: enabled)])], [enable_gme=$enableval], [enable_gme=yes])
AC_ARG_ENABLE(notify,   [AS_HELP_STRING([--disable-notify  ], [disable notification-daemon support plugin (default: enabled)])], [enable_notify=$enableval], [enable_notify=yes])
AC_ARG_ENABLE(shellexec, [AS_HELP_STRING([--disable-shellexec], [disable shell commands plugin (default: enabled)])], [enable_shellexec=$enableval], [enable_shellexec=yes])
AC_ARG_ENABLE(musepack, [AS_HELP_STRING([--disable-musepack], [disable musepack plugin (default: enabled)])], [enable_musepack=$enableval], [enable_musepack=yes])
AC_ARG_ENABLE(wildmidi, [AS_HELP_STRING([--disable-wildmidi], [disable wildmidi plugin (default: enabled)])], [enable_wildmidi=$enableval], [enable_wildmidi=yes])
AC_ARG_ENABLE(tta,      [AS_HELP_STRING([--disable-tta     ], [disable tta plugin (default: enabled)])], [enable_tta=$enableval], [enable_tta=yes])
AC_ARG_ENABLE(dca,      [AS_HELP_STRING([--disable-dca     ], [disable dca (DTS audio)  plugin (default: enabled)])], [enable_dca=$enableval], [enable_dca=yes])
AC_ARG_ENABLE(aac,      [AS_HELP_STRING([--disable-aac     ], [disable AAC decoder based on FAAD2 (default: enabled)])], [enable_aac=$enableval], [enable_aac=yes])
AC_ARG_ENABLE(mms,      [AS_HELP_STRING([--disable-mms     ], [disable MMS streaming vfs plugin (default: enabled)])], [enable_mms=$enableval], [enable_mms=yes])
AC_ARG_ENABLE(staticlink, [AS_HELP_STRING([--enable-staticlink], [link everything statically (default: disabled)])], [enable_staticlink=$enableval], [enable_staticlink=no])
AC_ARG_ENABLE(portable, [AS_HELP_STRING([--enable-portable ], [make portable build (default: disabled, opts: yes,no,full)])], [enable_portable=$enableval], [enable_portable=no])
AC_ARG_ENABLE(src,      [AS_HELP_STRING([--enable-src      ], [build libsamplerate (SRC) plugin (default: auto)])], [enable_src=$enableval], [enable_src=yes])
AC_ARG_ENABLE(m3u,      [AS_HELP_STRING([--enable-m3u      ], [build m3u plugin (default: auto)])], [enable_m3u=$enableval], [enable_m3u=yes])
AC_ARG_ENABLE(vfs-zip,      [AS_HELP_STRING([--enable-vfs-zip      ], [build vfs_zip plugin (default: auto)])], [enable_vfs_zip=$enableval], [enable_vfs_zip=yes])
AC_ARG_ENABLE(converter,      [AS_HELP_STRING([--enable-converter      ], [build converter plugin (default: auto)])], [enable_converter=$enableval], [enable_converter=yes])
AC_ARG_ENABLE(artwork-imlib2, [AS_HELP_STRING([--enable-artwork-imlib2      ], [use imlib2 in artwork plugin (default: auto)])], [enable_artwork_imlib2=$enableval], [enable_artwork_imlib2=yes])
AC_ARG_ENABLE(medialib, [AS_HELP_STRING([--enable-medialib      ], [build medialibrary plugin (default: auto)])], [enable_medialib=$enableval], [enable_medialib=yes])
AC_ARG_ENABLE(dumb,      [AS_HELP_STRING([--enable-dumb      ], [build DUMB plugin (default: auto)])], [enable_dumb=$enableval], [enable_dumb=yes])
AC_ARG_ENABLE(shn,      [AS_HELP_STRING([--enable-shn      ], [build SHN plugin (default: auto)])], [enable_shn=$enableval], [enable_shn=yes])
AC_ARG_ENABLE(psf,      [AS_HELP_STRING([--enable-psf      ], [build AOSDK-based PSF(,QSF,SSF,DSF) plugin (default: auto)])], [enable_psf=$enableval], [enable_psf=yes])
AC_ARG_ENABLE(mono2stereo,      [AS_HELP_STRING([--enable-mono2stereo      ], [build mono2stereo DSP plugin (default: auto)])], [enable_mono2stereo=$enableval], [enable_mono2stereo=yes])

if test "x$enable_staticlink" != "xno" ; then
    AC_DEFINE_UNQUOTED([STATICLINK], [1], [Define if building static version])
    STATICLINK=yes
fi

dnl check for yasm
AC_CHECK_PROG(HAVE_YASM, yasm, yes, no)

dnl check for libdl
AC_CHECK_LIB([dl], [main], [HAVE_DL=yes;DL_LIBS="-ldl";AC_SUBST(DL_LIBS)])

dnl check libsocket (OpenIndiana)
AC_CHECK_LIB([socket], [main], [HAVE_SOCKET=yes;DL_LIBS="-lsocket";AC_SUBST(DL_LIBS)])
dnl check for seperate alloca.h (OpenIndiana)
AC_CHECK_HEADER([alloca.h],[],[alloca.h not found.])

if test "x$enable_portable" != "xno" && test "x$enable_staticlink" != "xno" ; then
    AC_DEFINE_UNQUOTED([PORTABLE], [1], [Define if building portable version])
    PORTABLE=yes


    if test "x$enable_portable" == "xfull" ; then
        AC_DEFINE_UNQUOTED([PORTABLE_FULL], [1], [Define if portable version should keep configs in app folder])
        PORTABLE_FULL=yes
    fi

    PREFIXFLAGS="-DPREFIX=donotuse -DLIBDIR=donotuse -DDOCDIR=donotuse -I./$LIB/include -I../../$LIB/include"
else
    PREFIXFLAGS=" -DLIBDIR=\\\"$libdir\\\" -DPREFIX=\\\"$prefix\\\" -DDOCDIR=\\\"$docdir\\\""
fi

CXXFLAGS="$CXXFLAGS $INSANE_CXXFLAGS -D_GNU_SOURCE $PREFIXFLAGS"
CFLAGS="$CFLAGS $INSANE_CFLAGS -D_GNU_SOURCE $PREFIXFLAGS"

if test "x$enable_staticlink" != "xno" ; then
    HAVE_ZLIB=yes
    ZLIB_LIBS="../../$LIB/lib/libz.a"
else
    AC_CHECK_LIB([z], [main], [HAVE_ZLIB=yes])
    ZLIB_LIBS="-lz"
fi
AC_SUBST(ZLIB_LIBS)

if test "x$enable_staticlink" != "xno" ; then
    HAVE_ZIP=yes
    ZIP_LIBS="../../$LIB/lib/libzip.a ../../$LIB/lib/libz.a"
else
    AC_CHECK_LIB([zip], [main], [HAVE_ZIP=yes])
    ZIP_LIBS="-lzip"
fi
AC_SUBST(ZIP_LIBS)

if test "x$enable_gtk3" == "xyes" ; then
    PKG_CHECK_MODULES(GTK3_DEPS, gtk+-3.0 >= 3.0 gthread-2.0 glib-2.0, HAVE_GTK3=yes, HAVE_GTK3=no)
else
    HAVE_GTK3=no
fi

if test "x$enable_gtk2" == "xyes" ; then
    PKG_CHECK_MODULES(GTK2_DEPS, gtk+-2.0 >= 2.12 gthread-2.0 glib-2.0, HAVE_GTK2=yes, HAVE_GTK2=no)
else
    HAVE_GTK2=no
fi

if test "x$enable_alsa" != "xno" ; then
    PKG_CHECK_MODULES(ALSA_DEPS, alsa, HAVE_ALSA=yes, HAVE_ALSA=no)
fi

if test "x$enable_ffmpeg" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    FFMPEG_DEPS_LIBS="../../$LIB/lib/libavcodec.a -lpthread ../../$LIB/lib/libavformat.a ../../$LIB/lib/libavcodec.a ../../$LIB/lib/libavutil.a ../../$LIB/lib/libavcore.a -lm ../../$LIB/lib/libz.a "
    AC_SUBST(FFMPEG_DEPS_LIBS)
    HAVE_FFMPEG=yes
else
    PKG_CHECK_MODULES(FFMPEG_DEPS, libavcodec >= 51.0.0 libavutil libavformat >= 52.0.0, HAVE_FFMPEG=yes, HAVE_FFMPEG=no)
fi
fi

if test "x$enable_staticlink" != "xno" ; then
    HAVE_DBUS=yes
    DBUS_DEPS_LIBS="../../$LIB/lib/libdbus-1.a ../../$LIB/lib/libexpat.a -lrt"
    DBUS_DEPS_CFLAGS="-I../../$LIB/include/dbus-1"
    AC_SUBST(DBUS_DEPS_LIBS)
else
    PKG_CHECK_MODULES(DBUS_DEPS, dbus-1, HAVE_DBUS=yes, HAVE_DBUS=no)
fi

if test "x$enable_pulse" != "xno" ; then
    PKG_CHECK_MODULES(PULSE_DEPS, libpulse-simple, HAVE_PULSE=yes, HAVE_PULSE=no)
fi

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
if test "x$enable_staticlink" != "xno" ; then
    HAVE_CURL=yes
    CURL_LIBS="../../$LIB/lib/libcurl.a -lrt"
    AC_SUBST(CURL_LIBS)
else
    AC_CHECK_LIB([curl], [main], [HAVE_CURL=yes])
    CURL_LIBS="-lcurl"
    AC_SUBST(CURL_LIBS)
fi

dnl mpgmad plugin

if test "x$enable_mpgmad" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    HAVE_MPGMAD=yes
    MAD_LIBS="../../$LIB/lib/libmad.a"
    AC_SUBST(MAD_LIBS)
else
    AC_CHECK_LIB([mad], [main], [HAVE_LIBMAD=yes])
    if test "x$HAVE_LIBMAD" = "xyes" ; then
        HAVE_MPGMAD=yes
        MAD_LIBS="-lmad"
        AC_SUBST(MAD_LIBS)
    fi
fi
fi

dnl vorbis plugin
if test "x$enable_vorbis" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    HAVE_VORBISPLUGIN=yes
    VORBIS_LIBS="../../$LIB/lib/libogg.a ../../$LIB/lib/libvorbis.a ../../$LIB/lib/libvorbisenc.a ../../$LIB/lib/libvorbisfile.a"
    AC_SUBST(VORBIS_LIBS)
else
    AC_CHECK_LIB([vorbis], [main], [HAVE_VORBIS=yes])
    AC_CHECK_LIB([vorbisfile], [main], [HAVE_VORBISFILE=yes])
    if test "x$HAVE_VORBIS" = "xyes" && test "x$HAVE_VORBISFILE" = "xyes" ; then
        HAVE_VORBISPLUGIN=yes
        VORBIS_LIBS="-lvorbis -lvorbisfile"
        AC_SUBST(VORBIS_LIBS)
    fi
fi
fi

dnl flac plugin
if test "x$enable_flac" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    HAVE_FLACPLUGIN=yes
    FLAC_LIBS="../../$LIB/lib/libFLAC.a ../../$LIB/lib/libogg.a"
    AC_SUBST(FLAC_LIBS)
else
    AC_CHECK_LIB([FLAC], [main], [HAVE_FLAC=yes])
    if test "x$HAVE_FLAC" = "xyes" ; then
        HAVE_FLACPLUGIN=yes
        FLAC_LIBS="-lFLAC"
        AC_SUBST(FLAC_LIBS)
    fi
fi
fi

dnl wavpack plugin
if test "x$enable_wavpack" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    HAVE_WAVPACKPLUGIN=yes
    WAVPACK_LIBS="../../$LIB/lib/libwavpack.a"
    AC_SUBST(WAVPACK_LIBS)
else
    AC_CHECK_LIB([wavpack], [main], [HAVE_WAVPACK=yes])
    if test "x$HAVE_WAVPACK" = "xyes" ; then
        HAVE_WAVPACKPLUGIN=yes
        WAVPACK_LIBS="-lwavpack"
        AC_SUBST(WAVPACK_LIBS)
    fi
fi
fi

dnl libsndfile plugin
if test "x$enable_sndfile" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    HAVE_SNDFILEPLUGIN=yes
    SNDFILE_LIBS="../../$LIB/lib/libsndfile.a"
    AC_SUBST(SNDFILE_LIBS)
else
    AC_CHECK_LIB([sndfile], [main], [HAVE_SNDFILE=yes])
    if test "x$HAVE_SNDFILE" = "xyes" ; then
        HAVE_SNDFILEPLUGIN=yes
        SNDFILE_LIBS="-lsndfile"
        AC_SUBST(SNDFILE_LIBS)
    fi
fi
fi

dnl vfs_curl plugin
if test "x$enable_vfs_curl" != "xno" ; then
    if test "x$HAVE_CURL" = "xyes" ; then
        HAVE_VFS_CURL=yes
        VFS_CURL_LIBS="$CURL_LIBS"
        AC_SUBST(VFS_CURL_LIBS)
    fi
fi

dnl cdda plugin
if test "x$enable_cdda" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    HAVE_CDDAPLUGIN=yes
    CDDA_LIBS="../../$LIB/lib/libcdio.a ../../$LIB/lib/libudf.a ../../$LIB/lib/libiso9660.a ../../$LIB/lib/libcddb.a"
    AC_SUBST(CDDA_LIBS)
else
    AC_CHECK_LIB([cdio], [main], [HAVE_CDIO=yes])
    AC_CHECK_LIB([cddb], [main], [HAVE_CDDB=yes])
    if test "x$HAVE_CDIO" = "xyes" && test "x$HAVE_CDDB" = "xyes" ; then
        HAVE_CDDAPLUGIN=yes
        CDDA_LIBS="-lcdio -lcddb"
        AC_SUBST(CDDA_LIBS)
    fi
fi
fi

dnl gtkui plugin
if test "x$enable_gtkui" != "xno" ; then
    if test "x$HAVE_GTK2" == "xyes" || test "x$HAVE_GTK3" == "xyes" ; then
        HAVE_GTKUI=yes
    fi
fi

if test "x$enable_alsa" != "xno" ; then
    if test "x$HAVE_ALSA" = "xyes" ; then
        HAVE_ALSAPLUGIN=yes
    fi
fi

if test "x$enable_ffmpeg" != "xno" ; then
if test "x$enable_staticlink" = "xno" ; then
    if test "x$HAVE_FFMPEG" = "xyes" ; then
        HAVE_FFMPEGPLUGIN=yes
        AC_CHECK_HEADER([ffmpeg/avformat.h], FFMPEG_DEPS_CFLAGS="$FFMPEG_DEPS_CFLAGS -D FFMPEG_OLD")
    fi
else
    HAVE_FFMPEGPLUGIN=yes
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

dnl shellexec plugin
if test "x$enable_shellexec" != "xno" ; then
    HAVE_SHELLEXEC=yes
fi


if test "x$enable_artwork" != "xno" ; then
    if test "x$enable_staticlink" != "xno" ; then
        HAVE_JPEG=yes
        JPEG_DEPS_LIBS="../../$LIB/lib/libjpeg.a"
        AC_SUBST(JPEG_DEPS_LIBS)
        HAVE_PNG=yes
        PNG_DEPS_LIBS="../../$LIB/lib/libpng.a ../../$LIB/lib/libz.a"
        AC_SUBST(PNG_DEPS_LIBS)
        HAVE_IMLIB2=no
    else
        if test "x$enable_artwork_imlib2" != "xno" ; then
            PKG_CHECK_MODULES(IMLIB2_DEPS, imlib2, HAVE_IMLIB2=yes, HAVE_IMLIB2=no)
        fi

        if test "x$enable_artwork_imlib2" == "xno" || test "x$HAVE_IMLIB2" == "xno" ; then
            AC_CHECK_LIB([jpeg], [main], [HAVE_JPEG=yes;JPEG_DEPS_LIBS="-ljpeg";AC_SUBST(JPEG_DEPS_LIBS)])
            AC_CHECK_LIB([png], [main], [HAVE_PNG=yes;PNG_DEPS_LIBS="-lpng -lz";AC_SUBST(PNG_DEPS_LIBS)])
        fi
    fi

    if test "x$HAVE_VFS_CURL" = "xyes" && ( test "x$HAVE_JPEG" == "xyes" || test "x$HAVE_IMLIB2" == "xyes" ) ; then
        HAVE_ARTWORK=yes
    fi
fi

if test "x$enable_adplug" != "xno" ; then
    HAVE_ADPLUG=yes
fi

if test "x$enable_ffap" != "xno" && ( test "x$HAVE_YASM" == "xyes" || test "x$APE_USE_YASM" != "xyes" ) ; then
    HAVE_FFAP=yes
fi

if test "x$enable_sid" != "xno" ; then
    HAVE_SID=yes
fi

if test "x$enable_nullout" != "xno" ; then
    HAVE_NULLOUT=yes
fi

if test "x$enable_src" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    LIBSAMPLERATE_DEPS_LIBS="../../$LIB/lib/libsamplerate.a -lpthread ${DL_LIBS}"
    AC_SUBST(LIBSAMPLERATE_DEPS_LIBS)
    HAVE_DSP_SRC=yes
else
    PKG_CHECK_MODULES(LIBSAMPLERATE_DEPS, samplerate, HAVE_LIBSAMPLERATE=yes, HAVE_LIBSAMPLERATE=no)
    if test "x$HAVE_LIBSAMPLERATE" = "xyes" ; then
        HAVE_DSP_SRC=yes
    fi
fi
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

if test "x$HAVE_DBUS" = "xyes" && test "x$enable_notify" != "xno" ; then
    HAVE_NOTIFY=yes
    NOTIFY_LIBS="$DBUS_DEPS_LIBS"
    NOTIFY_CFLAGS="$DBUS_DEPS_CFLAGS"
    AC_SUBST(NOTIFY_LIBS)
    AC_SUBST(NOTIFY_CFLAGS)
fi

if test "x$enable_musepack" != "xno" ; then
    HAVE_MUSEPACK=yes
fi

if test "x$enable_wildmidi" != "xno" ; then
    HAVE_WILDMIDI=yes
fi

if test "x$enable_tta" != "xno" ; then
    HAVE_TTA=yes
fi

if test "x$enable_dca" != "xno" ; then
    HAVE_DCA=yes
fi

if test "x$enable_aac" != "xno" ; then
if test "x$enable_staticlink" != "xno" ; then
    FAAD2_LIBS="../../$LIB/lib/libfaad.a"
    AC_SUBST(FAAD2_LIBS)
    HAVE_AAC=yes
else
    AC_CHECK_LIB([faad], [main], [HAVE_FAAD=1])
    if test ${HAVE_FAAD} ; then
        FAAD2_LIBS="-lfaad"
        AC_SUBST(FAAD2_LIBS)
        HAVE_AAC=yes
    fi
fi
fi

if test "x$enable_mms" != "xno" ; then
    LIBMMS_LIBS=""
    AC_SUBST(LIBMMS_LIBS)
    HAVE_MMS=yes
fi

if test "x$enable_m3u" != "xno" ; then
    HAVE_M3U=yes
fi

if test "x$enable_vfs_zip" != "xno" ; then
    if test "x$HAVE_ZLIB" = "xyes" && test "x$HAVE_ZIP" = "xyes" ; then
        HAVE_VFS_ZIP=yes
    fi
fi

if test "x$enable_converter" != "xno" ; then
    if test "x$HAVE_GTK2" = "xyes" || test "x$HAVE_GTK3" = "xyes" ; then
        HAVE_CONVERTER=yes
    fi
fi

if test "x$enable_medialib" != "xno" ; then
    HAVE_MEDIALIB=yes
fi

if test "x$enable_dumb" != "xno" ; then
    HAVE_DUMB=yes
fi

if test "x$enable_shn" != "xno" ; then
    HAVE_SHN=yes
fi

if test "x$enable_psf" != "xno" ; then
    HAVE_PSF=yes
fi

if test "x$enable_mono2stereo" != "xno" ; then
    HAVE_MONO2STEREO=yes
fi

PLUGINS_DIRS="plugins/lastfm plugins/mpgmad plugins/vorbis plugins/flac plugins/wavpack plugins/sndfile plugins/vfs_curl plugins/cdda plugins/gtkui plugins/alsa plugins/ffmpeg plugins/hotkeys plugins/oss plugins/artwork plugins/adplug plugins/ffap plugins/sid plugins/nullout plugins/supereq plugins/vtx plugins/gme plugins/pulse plugins/notify plugins/musepack plugins/wildmidi plugins/tta plugins/dca plugins/aac plugins/mms plugins/shellexec plugins/dsp_libsrc plugins/m3u plugins/vfs_zip plugins/converter plugins/dumb plugins/shn plugins/ao plugins/mono2stereo plugins/medialib"

AM_CONDITIONAL(HAVE_VORBIS, test "x$HAVE_VORBISPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_FLAC, test "x$HAVE_FLACPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_WAVPACK, test "x$HAVE_WAVPACKPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_SNDFILE, test "x$HAVE_SNDFILEPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_CDDA, test "x$HAVE_CDDAPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_GTKUI, test "x$HAVE_GTKUI" = "xyes")
AM_CONDITIONAL(HAVE_GTK2, test "x$HAVE_GTK2" = "xyes")
AM_CONDITIONAL(HAVE_GTK3, test "x$HAVE_GTK3" = "xyes")
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
AM_CONDITIONAL(HAVE_LASTFM, test "x$HAVE_LASTFM" = "xyes")
AM_CONDITIONAL(HAVE_VFS_CURL, test "x$HAVE_VFS_CURL" = "xyes")
AM_CONDITIONAL(HAVE_HOTKEYS, test "x$HAVE_HOTKEYS" = "xyes")
AM_CONDITIONAL(HAVE_MPGMAD, test "x$HAVE_MPGMAD" = "xyes")
AM_CONDITIONAL(HAVE_OSS, test "x$HAVE_OSS" = "xyes")
AM_CONDITIONAL(HAVE_SUPEREQ, test "x$HAVE_SUPEREQ" = "xyes")
AM_CONDITIONAL(HAVE_NOTIFY, test "x$HAVE_NOTIFY" = "xyes")
AM_CONDITIONAL(HAVE_SHELLEXEC, test "x$HAVE_SHELLEXEC" = "xyes")
AM_CONDITIONAL(HAVE_MUSEPACK, test "x$HAVE_MUSEPACK" = "xyes")
AM_CONDITIONAL(HAVE_WILDMIDI, test "x$HAVE_WILDMIDI" = "xyes")
AM_CONDITIONAL(HAVE_TTA, test "x$HAVE_TTA" = "xyes")
AM_CONDITIONAL(HAVE_DCA, test "x$HAVE_DCA" = "xyes")
AM_CONDITIONAL(HAVE_AAC, test "x$HAVE_AAC" = "xyes")
AM_CONDITIONAL(HAVE_MMS, test "x$HAVE_MMS" = "xyes")
AM_CONDITIONAL(STATICLINK, test "x$STATICLINK" = "xyes")
AM_CONDITIONAL(PORTABLE, test "x$PORTABLE" = "xyes")
AM_CONDITIONAL(PORTABLE_FULL, test "x$PORTABLE_FULL" = "xyes")
AM_CONDITIONAL(HAVE_DSP_SRC, test "x$HAVE_DSP_SRC" = "xyes")
AM_CONDITIONAL(HAVE_M3U, test "x$HAVE_M3U" = "xyes")
AM_CONDITIONAL(HAVE_VFS_ZIP, test "x$HAVE_VFS_ZIP" = "xyes")
AM_CONDITIONAL(HAVE_CONVERTER, test "x$HAVE_CONVERTER" = "xyes")
AM_CONDITIONAL(HAVE_IMLIB2, test "x$HAVE_IMLIB2" = "xyes")
AM_CONDITIONAL(HAVE_JPEG, test "x$HAVE_JPEG" = "xyes")
AM_CONDITIONAL(HAVE_PNG, test "x$HAVE_PNG" = "xyes")
AM_CONDITIONAL(HAVE_YASM, test "x$HAVE_YASM" = "xyes")
AM_CONDITIONAL(HAVE_MEDIALIB, test "x$HAVE_MEDIALIB" = "xyes")
AM_CONDITIONAL(HAVE_DUMB, test "x$HAVE_DUMB" = "xyes")
AM_CONDITIONAL(HAVE_PSF, test "x$HAVE_PSF" = "xyes")
AM_CONDITIONAL(HAVE_SHN, test "x$HAVE_SHN" = "xyes")
AM_CONDITIONAL(HAVE_MONO2STEREO, test "x$HAVE_MONO2STEREO" = "xyes")

AC_SUBST(PLUGINS_DIRS)

if test "x$USE_MAINTAINER_MODE" = "xyes" ; then
echo
echo "Build Configuration:"
echo
echo "* Maintainer Mode:        $USE_MAINTAINER_MODE"
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
PRINT_PLUGIN_INFO([cdda],[cd audio player],[test "x$HAVE_CDDAPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([gtkui],[GTK user interface],[test "x$HAVE_GTKUI" = "xyes"])
PRINT_PLUGIN_INFO([hotkeys],[Global hotkeys support],[test "x$HAVE_HOTKEYS" = "xyes"])
PRINT_PLUGIN_INFO([ffmpeg],[ffmpeg codecs],[test "x$HAVE_FFMPEGPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([oss],[oss output plugin],[test "x$HAVE_OSS" = "xyes"])
PRINT_PLUGIN_INFO([pulse],[PulseAudio output plugin],[test "x$HAVE_PULSEPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([artwork],[Cover art plugin],[test "x$HAVE_ARTWORK" = "xyes"])
PRINT_PLUGIN_INFO([supereq],[Equalizer based on Super EQ library by Naoki Shibata],[test "x$HAVE_SUPEREQ" = "xyes"])
PRINT_PLUGIN_INFO([notify],[notification-daemon support plugin],[test "x$HAVE_NOTIFY" = "xyes"])
PRINT_PLUGIN_INFO([shellexec],[shell commands plugin],[test "x$HAVE_SHELLEXEC" = "xyes"])
PRINT_PLUGIN_INFO([musepack],[musepack player plugin],[test "x$HAVE_MUSEPACK" = "xyes"])
PRINT_PLUGIN_INFO([wildmidi],[WildMidi player plugin],[test "x$HAVE_WILDMIDI" = "xyes"])
PRINT_PLUGIN_INFO([tta],[TTA player plugin],[test "x$HAVE_TTA" = "xyes"])
PRINT_PLUGIN_INFO([dca],[libdca (DTS Audio) player plugin],[test "x$HAVE_DCA" = "xyes"])
PRINT_PLUGIN_INFO([aac],[AAC player (m4a, aac, mp4) based on FAAD2],[test "x$HAVE_AAC" = "xyes"])
PRINT_PLUGIN_INFO([mms],[mms streaming support],[test "x$HAVE_MMS" = "xyes"])
PRINT_PLUGIN_INFO([dsp_src],[High quality samplerate conversion using libsamplerate],[test "x$HAVE_DSP_SRC" = "xyes"])
PRINT_PLUGIN_INFO([m3u],[M3U and PLS playlist support],[test "x$HAVE_M3U" = "xyes"])
PRINT_PLUGIN_INFO([vfs_zip],[zip archive support],[test "x$HAVE_VFS_ZIP" = "xyes"])
PRINT_PLUGIN_INFO([converter],[plugin for converting files to any formats],[test "x$HAVE_CONVERTER" = "xyes"])
PRINT_PLUGIN_INFO([medialib],[media library support plugin],[test "x$HAVE_MEDIALIB" = "xyes"])
PRINT_PLUGIN_INFO([psf],[PSF format plugin, using AOSDK],[test "x$HAVE_PSF" = "xyes"])
PRINT_PLUGIN_INFO([dumb],[DUMB module plugin, for MOD, S3M, etc],[test "x$HAVE_DUMB" = "xyes"])
PRINT_PLUGIN_INFO([shn],[SHN plugin based on xmms-shn],[test "x$HAVE_SHN" = "xyes"])
PRINT_PLUGIN_INFO([mono2stereo],[mono2stereo DSP plugin],[test "x$HAVE_MONO2STEREO" = "xyes"])

echo

AC_CONFIG_FILES([
Makefile
pixmaps/Makefile
icons/Makefile
plugins/gme/Makefile
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
plugins/shellexec/Makefile
plugins/musepack/Makefile
plugins/wildmidi/Makefile
plugins/tta/Makefile
plugins/dca/Makefile
plugins/aac/Makefile
plugins/mms/Makefile
plugins/dsp_libsrc/Makefile
plugins/m3u/Makefile
plugins/vfs_zip/Makefile
plugins/converter/Makefile
plugins/medialib/Makefile
plugins/dumb/Makefile
plugins/ao/Makefile
plugins/shn/Makefile
plugins/mono2stereo/Makefile
intl/Makefile
po/Makefile.in
deadbeef.desktop
])

AC_OUTPUT
