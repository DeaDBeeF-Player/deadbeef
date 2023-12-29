dnl Process this file with autoconf to produce a configure script.
AC_INIT([deadbeef], [devel])

AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar silent-rules subdir-objects no-dependencies])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE()
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([disable-static])
AC_PROG_CC(clang)
AC_PROG_CXX
AC_PROG_OBJC
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_SYS_LARGEFILE
dnl AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_C_BIGENDIAN
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.18])
PKG_PROG_PKG_CONFIG
AM_PROG_CC_C_O
AM_ICONV
IT_PROG_INTLTOOL([0.40.0])
GETTEXT_PACKAGE=deadbeef
AC_SUBST(GETTEXT_PACKAGE)

test "$prefix" = NONE && prefix=/usr/local

case "$host" in
  i686-apple-*)
    AC_DEFINE(ARCH_X86_32, 1, [architecture is x86 on OSX])
    LIB="static-deps/lib-x86-32-apple"
    YASM_FLAGS="-f macho32 -D ARCH_X86_32 -m x86 -DPREFIX"
    APE_USE_YASM=yes
    OS_OSX=yes
    ;;
  x86_64-apple-*)
    AC_DEFINE(ARCH_X86_64, 1, [architecture is x86_64 on OSX])
    LIB="static-deps/lib-x86-64-apple"
    YASM_FLAGS="-f macho64 -D ARCH_X86_64 -m amd64 -DPIC -DPREFIX"
    APE_USE_YASM=yes
    OS_OSX=yes
    ;;
  i386-*-* | i486-*-* | i586-*-* | i686-*-* | i86pc-*-*)
    AC_DEFINE(ARCH_X86_32, 1, [architecture is x86])
    LIB="static-deps/lib-x86-32"
    YASM_FLAGS="-f elf -D ARCH_X86_32 -m x86"
    APE_USE_YASM=yes
    ;;
  x86_64-*-* | amd64-*-*)
    AC_DEFINE(ARCH_X86_64, 1, [architecture is x86_64])
    LIB="static-deps/lib-x86-64"
    YASM_FLAGS="-f elf -D ARCH_X86_64 -m amd64"
    APE_USE_YASM=yes
    ;;
  powerpc-*-* )
    AC_DEFINE(ARCH_PPC_32, 1, [architecture is ppc32])
    LIB="static-deps/lib-ppc-32"
    ;;
  powerpc64-*-* )
    AC_DEFINE(ARCH_PPC_64, 1, [architecture is ppc64])
    LIB="static-deps/lib-ppc-64"
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

dnl Check for clang availability
AX_CHECK_COMPILER_FLAGS(-fblocks,[],[AC_MSG_ERROR(Clang is required to build DeaDBeeF. Please check the README file.)])

AC_ARG_ENABLE(nullout,  [AS_HELP_STRING([--disable-nullout ], [disable NULL output plugin (default: enabled)])], [enable_nullout=$enableval], [enable_nullout=yes])
AC_ARG_ENABLE(alsa,     [AS_HELP_STRING([--disable-alsa    ], [disable ALSA output plugin (default: enabled)])], [enable_alsa=$enableval], [enable_alsa=yes])
AC_ARG_ENABLE(oss,      [AS_HELP_STRING([--disable-oss     ], [disable Open Sound System output plugin (default: enabled)])], [enable_oss=$enableval], [enable_oss=yes])
AC_ARG_ENABLE(pipewire, [AS_HELP_STRING([--disable-pipewire   ], [disable Pipewire output plugin (default: enabled)])], [enable_pipewire=$enableval], [enable_pipewire=yes])
AC_ARG_ENABLE(pulse,    [AS_HELP_STRING([--disable-pulse   ], [disable PulseAudio output plugin (default: enabled)])], [enable_pulse=$enableval], [enable_pulse=yes])
AC_ARG_ENABLE(coreaudio,[AS_HELP_STRING([--disable-coreaudio], [disable CoreAudio output plugin (default: enabled)])], [enable_coreaudio=$enableval], [enable_coreaudio=yes])
AC_ARG_ENABLE(gtk3,     [AS_HELP_STRING([--disable-gtk3     ], [disable GTK3 version of gtkui plugin (default: enabled)])], [enable_gtk3=$enableval], [enable_gtk3=yes])
AC_ARG_ENABLE(gtk2,     [AS_HELP_STRING([--disable-gtk2     ], [disable GTK2 version of gtkui plugin (default: enabled)])], [enable_gtk2=$enableval], [enable_gtk2=yes])
AC_ARG_ENABLE(vfs_curl, [AS_HELP_STRING([--disable-vfs-curl], [disable HTTP streaming vfs plugin (default: enabled)])], [enable_vfs_curl=$enableval], [enable_vfs_curl=yes])
AC_ARG_ENABLE(libretro, [AS_HELP_STRING([--disable-libretro   ], [disable Libretro resampler plugin (default: enabled)])], [enable_libretro=$enableval], [enable_libretro=yes])
AC_ARG_ENABLE(lfm,      [AS_HELP_STRING([--disable-lfm     ], [disable last.fm/libre.fm scrobbler plugin (default: enabled)])], [enable_lfm=$enableval], [enable_lfm=yes])
AC_ARG_ENABLE(artwork,  [AS_HELP_STRING([--disable-artwork ], [disable album art loader plugin (default: enabled)])], [enable_artwork=$enableval], [enable_artwork=yes])
AC_ARG_ENABLE(artwork-network,  [AS_HELP_STRING([--disable-artwork-network ], [disable album art network loading support (default: enabled)])], [enable_artwork_network=$enableval], [enable_artwork_network=yes])
AC_ARG_ENABLE(supereq,  [AS_HELP_STRING([--disable-supereq ], [disable SuperEQ DSP plugin (default: enabled)])], [enable_supereq=$enableval], [enable_supereq=yes])
AC_ARG_ENABLE(sid,      [AS_HELP_STRING([--disable-sid     ], [disable commodore64 SID music player plugin (default: enabled)])], [enable_sid=$enableval], [enable_sid=yes])
AC_ARG_ENABLE(mp3,      [AS_HELP_STRING([--disable-mp3     ], [disable mp3 plugin (default: enabled)])], [enable_mp3=$enableval], [enable_mp3=yes])
AC_ARG_ENABLE(libmad,   [AS_HELP_STRING([--disable-libmad     ], [disable libmad support in mp3 plugin (default: auto)])], [enable_libmad=$enableval], [enable_libmad=yes])
AC_ARG_ENABLE(libmpg123,[AS_HELP_STRING([--disable-libmpg123     ], [disable libmpg123 support in mp3 plugin (default: auto)])], [enable_libmpg123=$enableval], [enable_libmpg123=yes])
AC_ARG_ENABLE(ffap,     [AS_HELP_STRING([--disable-ffap    ], [disable Monkey's Audio plugin (default: enabled)])], [enable_ffap=$enableval], [enable_ffap=yes])
AC_ARG_ENABLE(vtx,      [AS_HELP_STRING([--disable-vtx     ], [disable libayemy VTX ZX-Spectrum music player plugin (default: enabled)])], [enable_vtx=$enableval], [enable_vtx=yes])
AC_ARG_ENABLE(adplug,   [AS_HELP_STRING([--disable-adplug  ], [disable adplug plugin (default: enabled)])], [enable_adplug=$enableval], [enable_adplug=yes])
AC_ARG_ENABLE(hotkeys,  [AS_HELP_STRING([--disable-hotkeys ], [disable global hotkeys plugin (default: enabled)])], [enable_hotkeys=$enableval], [enable_hotkeys=yes])
AC_ARG_ENABLE(vorbis,   [AS_HELP_STRING([--disable-vorbis  ], [disable Ogg Vorbis player plugin (default: enabled)])], [enable_vorbis=$enableval], [enable_vorbis=yes])
AC_ARG_ENABLE(opus,   [AS_HELP_STRING([--disable-opus  ], [disable Ogg Opus player plugin (default: enabled)])], [enable_opus=$enableval], [enable_opus=yes])
AC_ARG_ENABLE(ffmpeg,   [AS_HELP_STRING([--disable-ffmpeg  ], [disable FFMPEG plugin for WMA, AMR, etc (default: enabled)])], [enable_ffmpeg=$enableval], [enable_ffmpeg=yes])
AC_ARG_ENABLE(flac,     [AS_HELP_STRING([--disable-flac    ], [disable FLAC player plugin (default: enabled)])], [enable_flac=$enableval], [enable_flac=yes])
AC_ARG_ENABLE(sndfile,  [AS_HELP_STRING([--disable-sndfile ], [disable libsndfile plugin for PCM wave files (default: enabled)])], [enable_sndfile=$enableval], [enable_sndfile=yes])
AC_ARG_ENABLE(wavpack,  [AS_HELP_STRING([--disable-wavpack ], [disable wavpack plugin (default: enabled)])], [enable_wavpack=$enableval], [enable_wavpack=yes])
AC_ARG_ENABLE(cdda,     [AS_HELP_STRING([--disable-cdda    ], [disable CD-Audio plugin (default: enabled)])], [enable_cdda=$enableval], [enable_cdda=yes])
AC_ARG_ENABLE(cdda-paranoia,     [AS_HELP_STRING([--disable-cdda-paranoia    ], [disable CD-Audio error correction during ripping (default: enabled)])], [enable_cdda_paranoia=$enableval], [enable_cdda_paranoia=yes])
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
AC_ARG_ENABLE(medialib, [AS_HELP_STRING([--enable-medialib      ], [build media library plugin (default: auto)])], [enable_medialib=$enableval], [enable_medialib=yes])
AC_ARG_ENABLE(dumb,      [AS_HELP_STRING([--enable-dumb      ], [build DUMB plugin (default: auto)])], [enable_dumb=$enableval], [enable_dumb=yes])
AC_ARG_ENABLE(shn,      [AS_HELP_STRING([--enable-shn      ], [build SHN plugin (default: auto)])], [enable_shn=$enableval], [enable_shn=yes])
AC_ARG_ENABLE(psf,      [AS_HELP_STRING([--enable-psf      ], [build AOSDK-based PSF(,QSF,SSF,DSF) plugin (default: auto)])], [enable_psf=$enableval], [enable_psf=yes])
AC_ARG_ENABLE(mono2stereo,      [AS_HELP_STRING([--enable-mono2stereo      ], [build mono2stereo DSP plugin (default: auto)])], [enable_mono2stereo=$enableval], [enable_mono2stereo=yes])
AC_ARG_ENABLE(shellexecui, [AS_HELP_STRING([--enable-shellexecui      ], [build shellexec GTK UI plugin (default: auto)])], [enable_shellexecui=$enableval], [enable_shellexecui=yes])
AC_ARG_ENABLE(alac, [AS_HELP_STRING([--enable-alac      ], [build ALAC plugin (default: auto)])], [enable_alac=$enableval], [enable_alac=yes])
AC_ARG_ENABLE(wma, [AS_HELP_STRING([--enable-wma      ], [build WMA plugin (default: auto)])], [enable_wma=$enableval], [enable_wma=yes])
AC_ARG_ENABLE(pltbrowser, [AS_HELP_STRING([--enable-pltbrowser      ], [build playlist browser gui plugin (default: auto)])], [enable_pltbrowser=$enableval], [enable_pltbrowser=yes])
AC_ARG_ENABLE(sc68, [AS_HELP_STRING([--enable-sc68     ], [build sc68 Atari ST and Amiga music player (default: auto)])], [enable_sc68=$enableval], [enable_sc68=yes])
AC_ARG_ENABLE(abstract_socket, [AS_HELP_STRING([--enable-abstract-socket      ], [use abstract UNIX socket for IPC (default: disabled)])], [enable_abstract_socket=$enableval], [enable_abstract_socket=no])
dnl AC_ARG_ENABLE(statusnotifier, [AS_HELP_STRING([--enable-statusnotifier], [KDE status notifier plugin for tray icon (default: disabled)])], [enable_statusnotifier=$enableval], [enable_statusnotifier=no])
AC_ARG_ENABLE(rgscanner, [AS_HELP_STRING([--disable-rgscanner], [build ReplayGain scanner plugin (default: enabled)])], [enable_rgscanner=$enableval], [enable_rgscanner=yes])
AC_ARG_ENABLE(soundtouch,   [AS_HELP_STRING([--disable-soundtouch  ], [disable soundtouch plugin (default: enabled)])], [enable_soundtouch=$enableval], [enable_soundtouch=yes])

AS_IF([test "${enable_staticlink}" != "no"], [
    AC_DEFINE_UNQUOTED([STATICLINK], [1], [Define if building static version])
    STATICLINK=yes
])

AS_IF([test "${enable_abstract_socket}" != "no"], [
    AC_DEFINE_UNQUOTED([USE_ABSTRACT_SOCKET_NAME], [1], [Define to use abstract socket name, without file])
    USE_ABSTRACT_SOCKET_NAME=yes
])

case "$host" in
  *-*-gnu )
    AS_IF([test "${enable_staticlink}" != "no"], [
        NOCPPLIB="-nostdlib -lstdc++"
    ], [
        NOCPPLIB=""
    ])
    AC_SUBST(NOCPPLIB)
    ;;
esac

dnl check for yasm
AC_CHECK_PROG(HAVE_YASM, yasm, yes, no)

dnl check for log2
AC_CHECK_LIB([m], [log2], AC_DEFINE([HAVE_LOG2], [], [Define to 1 if the system has log2 in libm]))

dnl check for mkstemps
dnl AC_DEFINE_UNQUOTED([HAVE_MKSTEMPS], [0], [Define if mkstemps is available])
AC_CACHE_CHECK([for mkstemps],
xpdf_cv_func_mkstemps,
[AC_TRY_LINK([#include <stdlib.h>
#include <unistd.h>],
[mkstemps("foo", 0);],
xpdf_cv_func_mkstemps=yes, xpdf_cv_func_mkstemps=no)])
if test "$xpdf_cv_func_mkstemps" = yes; then
  AC_DEFINE(HAVE_MKSTEMPS, 1, [Define to 1 if you have the `mkstemps' function.])
fi

dnl check for libdl
AC_CHECK_LIB([dl], [main], [HAVE_DL=yes;DL_LIBS="-ldl";AC_SUBST(DL_LIBS)])

dnl check libsocket (OpenIndiana)
AC_CHECK_LIB([socket], [main], [HAVE_SOCKET=yes;DL_LIBS="-lsocket";AC_SUBST(DL_LIBS)])
dnl check for separate alloca.h (OpenIndiana)
AC_CHECK_HEADER([alloca.h],[ALLOCA_H_CFLAGS="-DHAVE_ALLOCA_H";AC_SUBST(ALLOCA_H_CFLAGS)],[alloca.h not found.])
dnl check for syslimits.h (BSD)
AC_CHECK_HEADERS([sys/syslimits.h])
AC_CHECK_HEADERS([sys/cdefs.h])
AC_CHECK_HEADERS([sys/wait.h])

AS_IF([test "${enable_portable}" != "no" -a "${enable_staticlink}" != "no"], [
    AC_DEFINE_UNQUOTED([PORTABLE], [1], [Define if building portable version])
    PORTABLE=yes


    AS_IF([test "${enable_portable}" = "full"], [
        AC_DEFINE_UNQUOTED([PORTABLE_FULL], [1], [Define if portable version should keep configs in app folder])
        PORTABLE_FULL=yes
    ])

    PREFIXFLAGS="-DPREFIX=donotuse -DLIBDIR=donotuse -DDOCDIR=donotuse -I./$LIB/include -I../../$LIB/include"
], [
    PREFIXFLAGS=" -DLIBDIR=\\\"$libdir\\\" -DPREFIX=\\\"$prefix\\\" -DDOCDIR=\\\"$docdir\\\""
])

dnl HACK: using undocumented variable to check whether it's safe to define __EXTENSIONS__
test $ac_cv_safe_to_define___extensions__ = yes && CFLAGS_EXTENSIONS="-D__EXTENSIONS__"

CXXFLAGS="$CXXFLAGS $INSANE_CXXFLAGS -D_GNU_SOURCE $CFLAGS_EXTENSIONS $PREFIXFLAGS -DDDB_WARN_DEPRECATED=1"
CFLAGS="$CFLAGS $INSANE_CFLAGS -D_GNU_SOURCE $CFLAGS_EXTENSIONS $PREFIXFLAGS -DDDB_WARN_DEPRECATED=1"


AS_IF([test "${enable_staticlink}" != "no"], [
    HAVE_ZLIB=yes
    ZLIB_CFLAGS="-I../../$LIB/include"
    ZLIB_LIBS="../../$LIB/lib/libz.a"
    AC_SUBST(ZLIB_CFLAGS)
    AC_SUBST(ZLIB_LIBS)
], [
    AC_CHECK_LIB([z], [main], [HAVE_ZLIB=yes])
    ZLIB_LIBS="-lz"
    AC_SUBST(ZLIB_LIBS)
])

AS_IF([test "${enable_staticlink}" != "no"], [
    HAVE_ZIP=yes
    ZIP_CFLAGS="-I../../$LIB/include"
    ZIP_LIBS="../../$LIB/lib/libzip.a ../../$LIB/lib/libz.a"
    AC_SUBST(ZIP_CFLAGS)
    AC_SUBST(ZIP_LIBS)
], [
    PKG_CHECK_MODULES(ZIP, libzip, HAVE_ZIP=yes, HAVE_ZIP=no)
])

AS_IF([test "${enable_gtk3}" = "yes"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        GTK3_DEPS_CFLAGS="${GTK3_310_CFLAGS}"
        GTK3_DEPS_LIBS="${GTK3_310_LIBS}"
        AC_SUBST(GTK3_DEPS_CFLAGS)
        AC_SUBST(GTK3_DEPS_LIBS)
        HAVE_GTK3=yes
    ], [
       PKG_CHECK_MODULES(GTK3_DEPS, gtk+-3.0 >= 3.0 gthread-2.0 glib-2.0 gio-2.0, HAVE_GTK3=yes, HAVE_GTK3=no)
    ])
    AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])
    ], [
    HAVE_GTK3=no
])
AM_CONDITIONAL(OS_OSX, test "x$OS_OSX" = "xyes")


AS_IF([test "${enable_gtk2}" = "yes" -o "${enable_gtk3}" = "yes" -o "${enable_shellexec}" = "yes"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        JANSSON_CFLAGS="-I../../$LIB/include"
        JANSSON_LIBS=-ljansson
        AC_SUBST(JANSSON_CFLAGS)
        AC_SUBST(JANSSON_LIBS)
    ], [
        PKG_CHECK_MODULES(JANSSON, jansson)
    ])
])

AS_IF([test "${enable_staticlink}" != "no"], [
    ], [
    AC_CHECK_LIB([dispatch], [main], [HAVE_DISPATCH=yes])
])
DISPATCH_CFLAGS=-fblocks
DISPATCH_LIBS="-ldispatch -lBlocksRuntime"
AC_SUBST(DISPATCH_CFLAGS)
AC_SUBST(DISPATCH_LIBS)

AS_IF([test "${enable_gtk2}" = "yes"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        GTK2_DEPS_CFLAGS="${GTK2_216_CFLAGS}"
        GTK2_DEPS_LIBS="${GTK2_216_LIBS}"
        AC_SUBST(GTK2_DEPS_CFLAGS)
        AC_SUBST(GTK2_DEPS_LIBS)
        HAVE_GTK2=yes
    ], [
        PKG_CHECK_MODULES(GTK2_DEPS, gtk+-2.0 >= 2.16 gthread-2.0 glib-2.0, HAVE_GTK2=yes, HAVE_GTK2=no)
    ])
dnl    AC_CHECK_LIB([SM], [main], [HAVE_SM=yes;SM_LIBS="-lSM";AC_SUBST(SM_LIBS)])
dnl    AC_CHECK_LIB([ICE], [main], [HAVE_ICE=yes;ICE_LIBS="-lICE";AC_SUBST(ICE_LIBS)])
], [
    HAVE_GTK2=no
])

AS_IF([test "${enable_alsa}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_ALSA=yes
        ALSA_DEPS_CFLAGS="-I../../$LIB/include"
        ALSA_DEPS_LIBS="-lasound"
        AC_SUBST(ALSA_DEPS_CFLAGS)
        AC_SUBST(ALSA_DEPS_LIBS)
    ],
    [
        PKG_CHECK_MODULES(ALSA_DEPS, alsa, HAVE_ALSA=yes, HAVE_ALSA=no)
    ])
])

AC_CHECK_HEADER([CoreAudio/AudioHardware.h], [found_coreaudio_h=yes], [found_coreaudio_h=no])

AS_IF([test "${enable_coreaudio}" != "no" -a "${found_coreaudio_h}" != "no"], [
  AC_DEFINE(HAVE_COREAUDIO, 1, [Build coreaudio support])
  HAVE_COREAUDIO=yes
  COREAUDIO_LIBS="-Wl,-framework,CoreAudio -Wl,-framework,AudioUnit"
])

AC_SUBST(COREAUDIO_LIBS)

AS_IF([test "${enable_ffmpeg}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        FFMPEG_DEPS_CFLAGS="-I../../$LIB/include"
        FFMPEG_DEPS_LIBS="../../$LIB/lib/libavcodec.a -lpthread ../../$LIB/lib/libavformat.a ../../$LIB/lib/libavcodec.a ../../$LIB/lib/libavutil.a -lm ../../$LIB/lib/libz.a ../../$LIB/lib/libopencore-amrnb.a ../../$LIB/lib/libopencore-amrwb.a ../../$LIB/lib/libopus.a"
        AC_SUBST(FFMPEG_DEPS_CFLAGS)
        AC_SUBST(FFMPEG_DEPS_LIBS)
        HAVE_FFMPEG=yes
    ], [
        PKG_CHECK_MODULES(FFMPEG_DEPS, libavcodec >= 51.0.0 libavutil libavformat >= 52.0.0, HAVE_FFMPEG=yes, HAVE_FFMPEG=no)
    ])
])

AS_IF([test "${enable_staticlink}" != "no"], [
    HAVE_DBUS=yes
    DBUS_DEPS_LIBS="../../$LIB/lib/libdbus-1.a ../../$LIB/lib/libexpat.a -lrt"
    DBUS_DEPS_CFLAGS="-I../../$LIB/include/dbus-1.0 -I../../$LIB/lib/dbus-1.0/include/"
    AC_SUBST(DBUS_DEPS_LIBS)
], [
    PKG_CHECK_MODULES(DBUS_DEPS, dbus-1, HAVE_DBUS=yes, HAVE_DBUS=no)
])

AS_IF([test "${enable_pulse}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_PULSE=yes
        PULSE_DEPS_LIBS="-lpulse-simple"
        PULSE_DEPS_CFLAGS="-I../../$LIB/include/"
        AC_SUBST(PULSE_DEPS_CFLAGS)
        AC_SUBST(PULSE_DEPS_LIBS)
    ], [
        PKG_CHECK_MODULES(PULSE_DEPS, libpulse-simple, HAVE_PULSE=yes, HAVE_PULSE=no)
    ])
])

AS_IF([test "${enable_pipewire}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_PIPEWIRE=yes
        PIPEWIRE_DEPS_LIBS="-lpipewire-0.3"
        PIPEWIRE_DEPS_CFLAGS="-D_REENTRANT -I../../$LIB/include/pipewire-0.3 -I../../$LIB/include/spa-0.2"
        AC_SUBST(PIPEWIRE_DEPS_CFLAGS)
        AC_SUBST(PIPEWIRE_DEPS_LIBS)
    ], [
        PKG_CHECK_MODULES(PIPEWIRE_DEPS, libpipewire-0.3, HAVE_PIPEWIRE=yes, HAVE_PIPEWIRE=no)
    ])
])

AS_IF([test "${enable_libretro}" != "no"], [
    HAVE_LIBRETRO=yes
])

AC_CHECK_HEADER([iconv.h],[],[iconv.h not found.])

AC_CHECK_LIB([iconv], [main], [have_iconv=yes], [have_iconv=no])
AS_IF([test "${have_iconv}" = "yes"], [
    ICONV_LIB="-liconv"
    AC_SUBST(ICONV_LIB)
    AC_DEFINE(HAVE_LIBICONV,1,[Use libiconv instead of glibc iconv])
])

AX_CHECK_COMPILER_FLAGS(-msse2, HAVE_SSE2=1, [], -Werror)
AS_IF([test "${HAVE_SSE2}"], [
    AC_DEFINE(HAVE_SSE2,1,[Support SSE2 instructions])
])

dnl curl lib
AS_IF([test "${enable_staticlink}" != "no"], [
    HAVE_CURL=yes
    CURL_LIBS="-lpthread -lrt -lcurl  ../../$LIB/lib/libz.a"
    CURL_CFLAGS="-I../../$LIB/include"
    AC_SUBST(CURL_LIBS)
    AC_SUBST(CURL_CFLAGS)
], [
    AC_CHECK_LIB([curl], [main], [HAVE_CURL=yes])
    CURL_LIBS="-lcurl"
    AC_SUBST(CURL_LIBS)
])

dnl mpg3 plugin

AS_IF([test "${enable_mp3}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_MP3=yes
        HAVE_LIBMAD=yes
        MAD_CFLAGS="-I../../$LIB/include"
        MAD_LIBS="../../$LIB/lib/libmad.a"
        AC_SUBST(MAD_CFLAGS)
        AC_SUBST(MAD_LIBS)
        HAVE_LIBMPG123=yes
        MPG123_CFLAGS="-I../../$LIB/include"
        MPG123_LIBS="../../$LIB/lib/libmpg123.a"
        AC_SUBST(MPG123_CFLAGS)
        AC_SUBST(MPG123_LIBS)
    ], [
        AS_IF([test "${enable_libmad}" != "no"], [
            AC_CHECK_LIB([mad], [main], [HAVE_LIBMAD=yes])
        ])
        AS_IF([test "${enable_libmpg123}" != "no"], [
            AC_CHECK_LIB([mpg123], [main], [HAVE_LIBMPG123=yes])
        ])
        HAVE_MP3=yes
        AS_IF([test "${HAVE_LIBMAD}" = "yes"], [
            MAD_LIBS="-lmad"
            AC_SUBST(MAD_LIBS)
        ])
        AS_IF([test "${HAVE_LIBMPG123}" = "yes"], [
            MPG123_LIBS="-lmpg123"
            AC_SUBST(MPG123_LIBS)
        ])
        AS_IF([test "${HAVE_LIBMAD}" != "yes" -a "${HAVE_LIBMPG123}" != "yes"], [
            echo "MP3 plugin is disabled: can't find either libmad of libmpg123"
            HAVE_MP3=no
        ])
    ])
])

dnl vorbis plugin
AS_IF([test "${enable_vorbis}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_VORBISPLUGIN=yes
        VORBIS_LIBS="-lvorbisfile -lvorbis -lm -logg"
        VORBIS_CFLAGS="-I../../$LIB/include"
        AC_SUBST(VORBIS_LIBS)
        AC_SUBST(VORBIS_CFLAGS)
    ], [
        AC_CHECK_LIB([vorbis], [main], [HAVE_VORBIS=yes])
        AC_CHECK_LIB([vorbisfile], [main], [HAVE_VORBISFILE=yes])
        AS_IF([test "$HAVE_VORBIS" = "yes" -a "${HAVE_VORBISFILE}" = "yes"], [
            HAVE_VORBISPLUGIN=yes
            VORBIS_LIBS="-lvorbis -lvorbisfile"
            AC_SUBST(VORBIS_LIBS)
        ])
    ])
])

dnl opus plugin
AS_IF([test "${enable_opus}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_OPUSPLUGIN=yes
        OPUS_LIBS="-lopusfile -lopus -lm -logg"
        OPUS_CFLAGS="-I../../$LIB/include -I../../$LIB/include/opus"
        AC_SUBST(OPUS_LIBS)
        AC_SUBST(OPUS_CFLAGS)
    ], [
        PKG_CHECK_MODULES(OPUS, opusfile, HAVE_OPUS=yes, HAVE_OPUS=no)
        AS_IF([test "$HAVE_OPUS" = "yes"], [
            HAVE_OPUSPLUGIN=yes
            AC_SUBST(OPUS_CFLAGS)
            AC_SUBST(OPUS_LIBS)
        ])
    ])
])
dnl libogg for oggedit
AS_IF([test "${enable_staticlink}" != "no"], [
    HAVE_OGG=yes
    OGG_LIBS="-logg"
    OGG_CFLAGS="-I../../$LIB/include"
    AC_SUBST(OGG_LIBS)
    AC_SUBST(OGG_CFLAGS)
], [
    AC_CHECK_LIB([ogg], [main], [HAVE_OGG=yes])
    AS_IF([test "$HAVE_OGG" = "yes"], [
        OGG_LIBS="-logg"
        AC_SUBST(OGG_LIBS)
    ])
])

AC_CHECK_LIB([ogg], [ogg_stream_flush_fill], [HAVE_OGG_STREAM_FLUSH_FILL=yes])

dnl flac plugin
AS_IF([test "${enable_flac}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_FLACPLUGIN=yes
        FLAC_CFLAGS="-I../../$LIB/include"
        FLAC_LIBS="-lFLAC"
        AC_SUBST(FLAC_CFLAGS)
        AC_SUBST(FLAC_LIBS)
    ], [
        AC_CHECK_LIB([FLAC], [main], [HAVE_FLAC=yes])
        AS_IF([test "${HAVE_FLAC}" = "yes"], [
            HAVE_FLACPLUGIN=yes
            FLAC_LIBS="-lFLAC"
            AC_SUBST(FLAC_LIBS)
        ])
    ])
])

dnl wavpack plugin
AS_IF([test "${enable_wavpack}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_WAVPACKPLUGIN=yes
        WAVPACK_CFLAGS="-I../../$LIB/include"
        WAVPACK_LIBS="../../$LIB/lib/libwavpack.a"
        AC_SUBST(WAVPACK_CFLAGS)
        AC_SUBST(WAVPACK_LIBS)
    ], [
        AC_CHECK_LIB([wavpack], [main], [HAVE_WAVPACK=yes])
        AS_IF([test "${HAVE_WAVPACK}" = "yes"], [
            HAVE_WAVPACKPLUGIN=yes
            WAVPACK_LIBS="-lwavpack"
            AC_SUBST(WAVPACK_LIBS)
        ])
    ])
])

dnl libsndfile plugin
AS_IF([test "${enable_sndfile}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_SNDFILEPLUGIN=yes
        SNDFILE_CFLAGS="-I../../$LIB/include"
        SNDFILE_LIBS="../../$LIB/lib/libsndfile.a"
        AC_SUBST(SNDFILE_CFLAGS)
        AC_SUBST(SNDFILE_LIBS)
    ], [
        AC_CHECK_LIB([sndfile], [main], [HAVE_SNDFILE=yes])
        AS_IF([test "${HAVE_SNDFILE}" = "yes"], [
            HAVE_SNDFILEPLUGIN=yes
            SNDFILE_LIBS="-lsndfile"
            AC_SUBST(SNDFILE_LIBS)
        ])
    ])
])

dnl vfs_curl plugin
AS_IF([test "${enable_vfs_curl}" != "no"], [
    AS_IF([test "${HAVE_CURL}" = "yes"], [
        HAVE_VFS_CURL=yes
    ])
])

dnl cdda plugin
AS_IF([test "${enable_cdda}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_CDDAPLUGIN=yes
        CDDA_LIBS="../../$LIB/lib/libcdio.a ../../$LIB/lib/libcdio_cdda.a ../../$LIB/lib/libcdio_paranoia.a ../../$LIB/lib/libudf.a ../../$LIB/lib/libiso9660.a ../../$LIB/lib/libcddb.a"
        CDDA_CFLAGS="-I../../$LIB/include -DUSE_PARANOIA -DUSE_PARANOIA_10_2"
        AC_SUBST(CDDA_LIBS)
        AC_SUBST(CDDA_CFLAGS)
    ], [
        AC_CHECK_LIB([cdio], [main], [HAVE_CDIO=yes])
        AC_CHECK_LIB([cddb], [main], [HAVE_CDDB=yes])
        AS_IF([test "${HAVE_CDIO}" = "yes" -a "${HAVE_CDDB}" = "yes"], [
            HAVE_CDDAPLUGIN=yes
            AC_CHECK_LIB([cdda_interface], [main], [HAVE_CDDA_INTERFACE=yes])
            AC_CHECK_LIB([cdda_paranoia], [main], [HAVE_CDDA_PARANOIA=yes])
            AS_IF([test "${enable_cdda_paranoia}" != "no" -a "${HAVE_CDDA_INTERFACE}" = "yes" -a "${HAVE_CDDA_PARANOIA}" = "yes"], [
                CDDA_CFLAGS="-DUSE_PARANOIA"
                AC_CHECK_HEADER([cdda/cdda_interface.h], [CDDA_CFLAGS="-DUSE_CDDA_SUBDIR $CDDA_CFLAGS"])
                AC_SUBST(CDDA_CFLAGS)
                CDDA_LIBS="-lcdio -lcdda_interface -lcdda_paranoia -lcddb"
            ] , [
                CDDA_LIBS="-lcdio -lcddb"
            ])
            AC_SUBST(CDDA_LIBS)
        ])
    ])
])

AS_IF([test "${enable_alsa}" != "no" -a "${HAVE_ALSA}" = "yes"], [
    HAVE_ALSAPLUGIN=yes
])

AS_IF([test "${enable_ffmpeg}" != "no"], [
    AS_IF([test "${enable_staticlink}" = "no"], [
        AS_IF([test "${HAVE_FFMPEG}" = "yes"], [
            HAVE_FFMPEGPLUGIN=yes
            AC_CHECK_HEADER([ffmpeg/avformat.h], FFMPEG_DEPS_CFLAGS="$FFMPEG_DEPS_CFLAGS -D FFMPEG_OLD")
        ])
    ], [
        HAVE_FFMPEGPLUGIN=yes
    ])
])

AS_IF([test "${enable_hotkeys}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HOTKEYS_CFLAGS="-I../../$LIB/include"
        HOTKEYS_LIBS="-lX11"
        AC_SUBST(HOTKEYS_CFLAGS)
        AC_SUBST(HOTKEYS_LIBS)
        HAVE_HOTKEYS=yes
    ],
    [
        AC_CHECK_HEADER([X11/Xlib.h], HAVE_XLIB_H=yes)
        AS_IF([test "${HAVE_XLIB_H}" = "yes"], [
            AS_IF([test "${HAVE_XLIB_H}" = "yes"], [
                HOTKEYS_LIBS="-lX11"
                AC_SUBST(HOTKEYS_LIBS)
                HAVE_HOTKEYS=yes
            ])
        ])
    ])
])

dnl no support for global hotkeys on osx, but plugin will work anyway
AS_IF([test "${OS_OSX}" = "yes"], [
    HAVE_HOTKEYS=yes
])

dnl *** OSS output (partly stolen from audacious)
AS_IF([test "${enable_oss}" != "no"], [
    have_oss=no

    AC_MSG_CHECKING(for OSS include dir)
    OSS_CFLAGS=""
    AS_IF([test -f "/etc/oss.conf"], [
        for i in `cat /etc/oss.conf`; do
            t=`echo $i | sed -e 's/OSSLIBDIR=//'`
            AS_IF([test "${i}" != "${t}"], [
                AS_IF([test -f "${t}/include/sys/soundcard.h"], [
                    OSS_CFLAGS="-I$t/include/sys"
                    AC_MSG_RESULT([$OSS_CFLAGS])
                    have_oss=yes
                ])
            ])
        done
    ], [
        AC_MSG_RESULT([not found])
    ])

    AS_IF([test "${have_oss}" != "yes"], [
        AC_MSG_CHECKING(for sys/soundcard.h)
        AC_CHECK_HEADERS(sys/soundcard.h)
        AS_IF([test "${ac_cv_header_sys_soundcard_h}" = "yes"], [
            have_oss=yes
            AC_MSG_RESULT([found])
        ], [
            AC_MSG_RESULT([not found])
        ])
    ])


    AS_IF([test "${have_oss}" = "yes"], [
        HAVE_OSS=yes
        AC_SUBST(OSS_CFLAGS)
    ])
])

AS_IF([test "${enable_pulseaudio}" != "no"], [
    AS_IF([test "${HAVE_PULSE}" = "yes"], [
        HAVE_PULSEPLUGIN=yes
    ])
])

AS_IF([test "${enable_pipewire}" != "no"], [
    AS_IF([test "${HAVE_PIPEWIRE}" = "yes"], [
        HAVE_PIPEWIRE_PLUGIN=yes
    ])
])

dnl lastfm plugin
AS_IF([test "${enable_lfm}" != "no"], [
    AS_IF([test "${HAVE_CURL}" = "yes" -a "${enable_lastfm}" != "no"], [
        HAVE_LASTFM=yes
    ])
])

dnl shellexec plugin
AS_IF([test "${enable_shellexec}" != "no"], [
    HAVE_SHELLEXEC=yes
])

AS_IF([test "${enable_shellexecui}" != "no" -a "${enable_shellexec}" != "no"], [
    AS_IF([test "${HAVE_GTK2}" = "yes" -o "${HAVE_GTK3}" = "yes"], [
        HAVE_SHELLEXECUI=yes
    ])
])

AS_IF([test "${enable_artwork}" != "no"], [
    AS_IF([test "${enable_artwork_network}" = "yes"], [
        ARTWORK_USE_VFS_CURL=yes
    ])

    AS_IF([test "${HAVE_VFS_CURL}" = "yes" -o "${enable_artwork_network}" = "no" ], [
        HAVE_ARTWORK=yes
    ])
])

AS_IF([test "${enable_adplug}" != "no"], [
    HAVE_ADPLUG=yes
])

AS_IF([test "${enable_ffap}" != "no"], [
    AS_IF([test "${HAVE_YASM}" = "yes" -o "${APE_USE_YASM}" != "yes"], [
        HAVE_FFAP=yes
    ])
])

AS_IF([test "${enable_sid}" != "no"], [
    HAVE_SID=yes
])

AS_IF([test "${enable_nullout}" != "no"], [
    HAVE_NULLOUT=yes
])

AS_IF([test "${enable_src}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        LIBSAMPLERATE_DEPS_CFLAGS="-I../../$LIB/include"
        LIBSAMPLERATE_DEPS_LIBS="../../$LIB/lib/libsamplerate.a -lpthread ${DL_LIBS}"
        AC_SUBST(LIBSAMPLERATE_DEPS_CFLAGS)
        AC_SUBST(LIBSAMPLERATE_DEPS_LIBS)
        HAVE_DSP_SRC=yes
    ], [
        PKG_CHECK_MODULES(LIBSAMPLERATE_DEPS, samplerate, HAVE_LIBSAMPLERATE=yes, HAVE_LIBSAMPLERATE=no)
        AS_IF([test "${HAVE_LIBSAMPLERATE}" = "yes"], [
            HAVE_DSP_SRC=yes
        ])
    ])
])

AS_IF([test "${enable_supereq}" != "no"], [
    HAVE_SUPEREQ=yes
])

AS_IF([test "${enable_vtx}" != "no"], [
    HAVE_VTX=yes
])

AS_IF([test "${enable_gme}" != "no" -a "${HAVE_ZLIB}" = "yes"], [
    HAVE_GME=yes
])

AS_IF([test "${HAVE_DBUS}" = "yes" -a "${enable_notify}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        HAVE_NOTIFY=yes
        NOTIFY_DEPS_CFLAGS="${GTK2_216_CFLAGS}"
        NOTIFY_DEPS_LIBS="${GDK_PIXBUF_LIBS}"
        AC_SUBST(NOTIFY_DEPS_CFLAGS)
        AC_SUBST(NOTIFY_DEPS_LIBS)
    ], [
        PKG_CHECK_MODULES(NOTIFY_DEPS, glib-2.0 >= 2.26 gio-2.0 gdk-pixbuf-2.0, HAVE_NOTIFY=yes, HAVE_NOTIFY=no)
    ])

    NOTIFY_LIBS="$DBUS_DEPS_LIBS"
    NOTIFY_CFLAGS="$DBUS_DEPS_CFLAGS"
    AC_SUBST(NOTIFY_LIBS)
    AC_SUBST(NOTIFY_CFLAGS)
])

AS_IF([test "${enable_musepack}" != "no"], [
    HAVE_MUSEPACK=yes
])

AS_IF([test "${enable_wildmidi}" != "no"], [
    HAVE_WILDMIDI=yes
])

AS_IF([test "${enable_tta}" != "no"], [
    HAVE_TTA=yes
])

AS_IF([test "${enable_dca}" != "no"], [
    HAVE_DCA=yes
])

AS_IF([test "${enable_aac}" != "no"], [
    AS_IF([test "${enable_staticlink}" != "no"], [
        FAAD2_CFLAGS="-I../../$LIB/include"
        FAAD2_LIBS="../../$LIB/lib/libfaad.a"
        AC_SUBST(FAAD2_CFLAGS)
        AC_SUBST(FAAD2_LIBS)
        HAVE_AAC=yes
    ], [
        AC_CHECK_LIB([faad], [main], [HAVE_FAAD=1])
        AS_IF([test "${HAVE_FAAD}"], [
            FAAD2_LIBS="-lfaad"
            AC_SUBST(FAAD2_LIBS)
            HAVE_AAC=yes
        ])
    ])
])

AS_IF([test "${enable_mms}" != "no"], [
    LIBMMS_LIBS=""
    AC_SUBST(LIBMMS_LIBS)
    HAVE_MMS=yes
])

AS_IF([test "${enable_m3u}" != "no"], [
    HAVE_M3U=yes
])

AS_IF([test "${enable_vfs_zip}" != "no"], [
    AS_IF([test "${HAVE_ZLIB}" = "yes" -a "${HAVE_ZIP}" = "yes"], [
        HAVE_VFS_ZIP=yes
    ])
])

AS_IF([test "${enable_converter}" != "no"], [
    AS_IF([test "${HAVE_GTK2}" = "yes" -o "${HAVE_GTK3}" = "yes"], [
        HAVE_CONVERTER=yes
    ])
])

AS_IF([test "${enable_medialib}" != "no"], [
    HAVE_MEDIALIB=yes
])

AS_IF([test "${enable_dumb}" != "no"], [
    HAVE_DUMB=yes
])

AS_IF([test "${enable_shn}" != "no"], [
    HAVE_SHN=yes
])

AS_IF([test "${enable_psf}" != "no" -a "${HAVE_ZLIB}" = "yes"], [
    HAVE_PSF=yes
])

AS_IF([test "${enable_mono2stereo}" != "no"], [
    HAVE_MONO2STEREO=yes
])

AS_IF([test "${enable_alac}" != "no"], [
    HAVE_ALAC=yes
])

AS_IF([test "${enable_wma}" != "no"], [
    HAVE_WMA=yes
])

AS_IF([test "${enable_pltbrowser}" != "no"], [
    HAVE_PLTBROWSER=yes
])

AS_IF([test "${enable_sc68}" != "no" -a "${HAVE_ZLIB}" = "yes"], [
    HAVE_SC68=yes
])

dnl AS_IF([test "${enable_statusnotifier}" != "no"], [
dnl     echo "Checking statusnotifier deps..."
dnl     AS_IF([test "${enable_staticlink}" != "no"], [
dnl         HAVE_STATUSNOTIFIER=yes
dnl         STATUSNOTIFIER_DEPS_CFLAGS="${GTK3_310_CFLAGS}"
dnl         STATUSNOTIFIER_DEPS_LIBS="${GTK3_310_LIBS}"
dnl         AC_SUBST(STATUSNOTIFIER_DEPS_CFLAGS)
dnl         AC_SUBST(STATUSNOTIFIER_DEPS_LIBS)
dnl     ], [
dnl         PKG_CHECK_MODULES(STATUSNOTIFIER_DEPS, glib-2.0 >= 2.26 gio-2.0 gdk-pixbuf-2.0, HAVE_STATUSNOTIFIER=yes, HAVE_STATUSNOTIFIER=no)
dnl     ])
dnl ])

AS_IF([test "${enable_rgscanner}" != "no"], [
    HAVE_RGSCANNER=yes
])

AS_IF([test "${enable_soundtouch}" != "no"], [
    HAVE_SOUNDTOUCH=yes
])

PLUGINS_DIRS="plugins/liboggedit plugins/libparser plugins/lastfm plugins/mp3 plugins/vorbis plugins/opus plugins/flac plugins/wavpack plugins/sndfile plugins/vfs_curl plugins/cdda plugins/gtkui plugins/alsa plugins/ffmpeg plugins/hotkeys plugins/oss plugins/artwork plugins/adplug plugins/ffap plugins/sid plugins/nullout plugins/supereq plugins/vtx plugins/gme plugins/pulse plugins/notify plugins/musepack plugins/wildmidi plugins/tta plugins/dca plugins/aac plugins/mms plugins/shellexec plugins/shellexecui plugins/dsp_libsrc plugins/m3u plugins/vfs_zip plugins/converter plugins/dumb plugins/shn plugins/psf plugins/mono2stereo plugins/alac plugins/wma plugins/pltbrowser plugins/coreaudio plugins/sc68 plugins/rg_scanner plugins/soundtouch plugins/medialib"

AM_CONDITIONAL(APE_USE_YASM, test "x$APE_USE_YASM" = "xyes")
AM_CONDITIONAL(HAVE_VORBIS, test "x$HAVE_VORBISPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_OPUS, test "x$HAVE_OPUSPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_OGG, test "x$HAVE_OGG" = "xyes")
AM_CONDITIONAL(HAVE_OGG_STREAM_FLUSH_FILL, test "x$HAVE_OGG_STREAM_FLUSH_FILL" = "xyes")
AM_CONDITIONAL(HAVE_FLAC, test "x$HAVE_FLACPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_WAVPACK, test "x$HAVE_WAVPACKPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_SNDFILE, test "x$HAVE_SNDFILEPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_CDDA, test "x$HAVE_CDDAPLUGIN" = "xyes")
AM_CONDITIONAL(USE_CDPARANOIA, test "x$HAVE_CDDAPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_GTK2, test "x$HAVE_GTK2" = "xyes")
AM_CONDITIONAL(HAVE_GTK3, test "x$HAVE_GTK3" = "xyes")
AM_CONDITIONAL(HAVE_ALSA, test "x$HAVE_ALSAPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_COREAUDIO, test "x$HAVE_COREAUDIO" = "xyes")
AM_CONDITIONAL(HAVE_FFMPEG, test "x$HAVE_FFMPEGPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_PIPEWIRE, test "x$HAVE_PIPEWIRE_PLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_PULSE, test "x$HAVE_PULSEPLUGIN" = "xyes")
AM_CONDITIONAL(HAVE_ARTWORK, test "x$HAVE_ARTWORK" = "xyes")
AM_CONDITIONAL(ARTWORK_USE_VFS_CURL, test "x$ARTWORK_USE_VFS_CURL" = "xyes")
AM_CONDITIONAL(HAVE_ADPLUG, test "x$HAVE_ADPLUG" = "xyes")
AM_CONDITIONAL(HAVE_FFAP, test "x$HAVE_FFAP" = "xyes")
AM_CONDITIONAL(HAVE_SID, test "x$HAVE_SID" = "xyes")
AM_CONDITIONAL(HAVE_NULLOUT, test "x$HAVE_NULLOUT" = "xyes")
AM_CONDITIONAL(HAVE_VTX, test "x$HAVE_VTX" = "xyes")
AM_CONDITIONAL(HAVE_GME, test "x$HAVE_GME" = "xyes")
AM_CONDITIONAL(HAVE_LIBRETRO, test "x$HAVE_LIBRETRO" = "xyes")
AM_CONDITIONAL(HAVE_LASTFM, test "x$HAVE_LASTFM" = "xyes")
AM_CONDITIONAL(HAVE_VFS_CURL, test "x$HAVE_VFS_CURL" = "xyes")
AM_CONDITIONAL(HAVE_HOTKEYS, test "x$HAVE_HOTKEYS" = "xyes")
AM_CONDITIONAL(HAVE_MP3, test "x$HAVE_MP3" = "xyes")
AM_CONDITIONAL(HAVE_LIBMAD, test "x$HAVE_LIBMAD" = "xyes")
AM_CONDITIONAL(HAVE_LIBMPG123, test "x$HAVE_LIBMPG123" = "xyes")
AM_CONDITIONAL(HAVE_OSS, test "x$HAVE_OSS" = "xyes")
AM_CONDITIONAL(HAVE_SUPEREQ, test "x$HAVE_SUPEREQ" = "xyes")
AM_CONDITIONAL(HAVE_NOTIFY, test "x$HAVE_NOTIFY" = "xyes")
AM_CONDITIONAL(HAVE_SHELLEXEC, test "x$HAVE_SHELLEXEC" = "xyes")
AM_CONDITIONAL(HAVE_SHELLEXECUI, test "x$HAVE_SHELLEXECUI" = "xyes")
AM_CONDITIONAL(HAVE_MUSEPACK, test "x$HAVE_MUSEPACK" = "xyes")
AM_CONDITIONAL(HAVE_WILDMIDI, test "x$HAVE_WILDMIDI" = "xyes")
AM_CONDITIONAL(HAVE_TTA, test "x$HAVE_TTA" = "xyes")
AM_CONDITIONAL(HAVE_DCA, test "x$HAVE_DCA" = "xyes")
AM_CONDITIONAL(HAVE_AAC, test "x$HAVE_AAC" = "xyes")
AM_CONDITIONAL(HAVE_MMS, test "x$HAVE_MMS" = "xyes")
AM_CONDITIONAL(HAVE_DSP_SRC, test "x$HAVE_DSP_SRC" = "xyes")
AM_CONDITIONAL(HAVE_M3U, test "x$HAVE_M3U" = "xyes")
AM_CONDITIONAL(HAVE_VFS_ZIP, test "x$HAVE_VFS_ZIP" = "xyes")
AM_CONDITIONAL(HAVE_CONVERTER, test "x$HAVE_CONVERTER" = "xyes")
AM_CONDITIONAL(HAVE_YASM, test "x$HAVE_YASM" = "xyes")
AM_CONDITIONAL(HAVE_MEDIALIB, test "x$HAVE_MEDIALIB" = "xyes")
AM_CONDITIONAL(HAVE_DUMB, test "x$HAVE_DUMB" = "xyes")
AM_CONDITIONAL(HAVE_PSF, test "x$HAVE_PSF" = "xyes")
AM_CONDITIONAL(HAVE_SHN, test "x$HAVE_SHN" = "xyes")
AM_CONDITIONAL(HAVE_MONO2STEREO, test "x$HAVE_MONO2STEREO" = "xyes")
dnl AM_CONDITIONAL(HAVE_SM, test "x$HAVE_SM" = "xyes")
dnl AM_CONDITIONAL(HAVE_ICE, test "x$HAVE_ICE" = "xyes")
AM_CONDITIONAL(HAVE_ALAC, test "x$HAVE_ALAC" = "xyes")
AM_CONDITIONAL(HAVE_WMA, test "x$HAVE_WMA" = "xyes")
AM_CONDITIONAL(HAVE_PLTBROWSER, test "x$HAVE_PLTBROWSER" = "xyes")
AM_CONDITIONAL(HAVE_SC68, test "x$HAVE_SC68" = "xyes")
AM_CONDITIONAL(STATICLINK, test "x$STATICLINK" = "xyes")
AM_CONDITIONAL(PORTABLE, test "x$PORTABLE" = "xyes")
AM_CONDITIONAL(PORTABLE_FULL, test "x$PORTABLE_FULL" = "xyes")
AM_CONDITIONAL(USE_ABSTRACT_SOCKET_NAME, test "x$USE_ABSTRACT_SOCKET_NAME" = "xyes")
AM_CONDITIONAL(HAVE_SSE2, test "${HAVE_SSE2}")
dnl AM_CONDITIONAL(HAVE_STATUSNOTIFIER, test "x$HAVE_STATUSNOTIFIER" = "xyes")
AM_CONDITIONAL(HAVE_RGSCANNER, test "x$HAVE_RGSCANNER" = "xyes")
AM_CONDITIONAL(HAVE_SOUNDTOUCH, test "x$HAVE_SOUNDTOUCH" = "xyes")

AC_SUBST(PLUGINS_DIRS)

AC_CONFIG_FILES([
Makefile
src/Makefile
shared/Makefile
shared/analyzer/Makefile
shared/scope/Makefile
shared/scriptable/Makefile
external/Makefile
external/ddb_dsp_libretro/Makefile
external/ddb_output_pw/Makefile
pixmaps/Makefile
icons/Makefile
plugins/liboggedit/Makefile
plugins/libparser/Makefile
plugins/gme/Makefile
plugins/alsa/Makefile
plugins/hotkeys/Makefile
plugins/lastfm/Makefile
plugins/ffap/Makefile
plugins/mp3/Makefile
plugins/vorbis/Makefile
plugins/opus/Makefile
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
plugins/dumb/Makefile
plugins/psf/Makefile
plugins/shn/Makefile
plugins/mono2stereo/Makefile
plugins/shellexecui/Makefile
plugins/alac/Makefile
plugins/wma/Makefile
plugins/pltbrowser/Makefile
plugins/sc68/Makefile
plugins/coreaudio/Makefile
plugins/rg_scanner/Makefile
plugins/soundtouch/Makefile
plugins/medialib/Makefile
po/Makefile.in
intl/Makefile
deadbeef.desktop
])

AC_OUTPUT

AS_IF([test "${USE_MAINTAINER_MODE}" = "yes"], [
echo
echo "Build Configuration:"
echo
echo "* Maintainer Mode:        $USE_MAINTAINER_MODE"
])

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

PRINT_PLUGIN_INFO([aac],[AAC player (m4a, aac, mp4) based on FAAD2],[test "x$HAVE_AAC" = "xyes"])
PRINT_PLUGIN_INFO([adplug],[adplug player (OPL2/OPL3 emulation)],[test "x$HAVE_ADPLUG" = "xyes"])
PRINT_PLUGIN_INFO([alac],[ALAC plugin],[test "x$HAVE_ALAC" = "xyes"])
PRINT_PLUGIN_INFO([alsa],[ALSA output],[test "x$HAVE_ALSA" = "xyes"])
PRINT_PLUGIN_INFO([artwork],[Cover art plugin],[test "x$HAVE_ARTWORK" = "xyes"])
PRINT_PLUGIN_INFO([cdda],[cd audio player],[test "x$HAVE_CDDAPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([converter],[plugin for converting files to any formats],[test "x$HAVE_CONVERTER" = "xyes"])
PRINT_PLUGIN_INFO([coreaudio],[CoreAudio output plugin],[test "x$HAVE_COREAUDIO" = "xyes"])
PRINT_PLUGIN_INFO([dca],[libdca (DTS Audio) player plugin],[test "x$HAVE_DCA" = "xyes"])
PRINT_PLUGIN_INFO([dsp_src],[High quality samplerate conversion using libsamplerate],[test "x$HAVE_DSP_SRC" = "xyes"])
PRINT_PLUGIN_INFO([dumb],[DUMB module plugin, for MOD, S3M, etc],[test "x$HAVE_DUMB" = "xyes"])
PRINT_PLUGIN_INFO([ffap],[Monkey's audio (APE) decoder],[test "x$HAVE_FFAP" = "xyes"])
PRINT_PLUGIN_INFO([ffmpeg],[ffmpeg codecs],[test "x$HAVE_FFMPEGPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([flac],[flac player],[test "x$HAVE_FLACPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([gme],[chiptune music player based on GME],[test "x$HAVE_GME" = "xyes"])
PRINT_PLUGIN_INFO([gtkui3],[GTK3 user interface],[test "x$HAVE_GTK3" = "xyes"])
PRINT_PLUGIN_INFO([gtkui],[GTK2 user interface],[test "x$HAVE_GTK2" = "xyes"])
PRINT_PLUGIN_INFO([hotkeys],[Local and global hotkeys support],[test "x$HAVE_HOTKEYS" = "xyes"])
PRINT_PLUGIN_INFO([lastfm],[last.fm scrobbler],[test "x$HAVE_LASTFM" = "xyes"])
PRINT_PLUGIN_INFO([libretro],[Libretro resampler plugin],[test "x$HAVE_LIBRETRO" = "xyes"])
PRINT_PLUGIN_INFO([m3u],[M3U and PLS playlist support],[test "x$HAVE_M3U" = "xyes"])
PRINT_PLUGIN_INFO([medialib],[Media library],[test "x$HAVE_MEDIALIB" = "xyes"])
PRINT_PLUGIN_INFO([mms],[mms streaming support],[test "x$HAVE_MMS" = "xyes"])
PRINT_PLUGIN_INFO([mono2stereo],[mono2stereo DSP plugin],[test "x$HAVE_MONO2STEREO" = "xyes"])
PRINT_PLUGIN_INFO([mp3],[mp3 plugin],[test "x$HAVE_MP3" = "xyes"])
PRINT_PLUGIN_INFO([    libmad],[libmad backend for mp3 plugin],[test "x$HAVE_LIBMAD" = "xyes"])
PRINT_PLUGIN_INFO([    libmpg123],[libmpg123 backend for mp3 plugin],[test "x$HAVE_LIBMPG123" = "xyes"])
PRINT_PLUGIN_INFO([musepack],[musepack player plugin],[test "x$HAVE_MUSEPACK" = "xyes"])
PRINT_PLUGIN_INFO([notify],[notification-daemon support plugin],[test "x$HAVE_NOTIFY" = "xyes"])
PRINT_PLUGIN_INFO([nullout],[NULL output],[test "x$HAVE_NULLOUT" = "xyes"])
PRINT_PLUGIN_INFO([opus],[ogg opus player],[test "x$HAVE_OPUSPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([oss],[oss output plugin],[test "x$HAVE_OSS" = "xyes"])
PRINT_PLUGIN_INFO([pltbrowser],[playlist browser gui plugin],[test "x$HAVE_PLTBROWSER" = "xyes"])
PRINT_PLUGIN_INFO([psf],[PSF player, using Audio Overload SDK],[test "x$HAVE_PSF" = "xyes"])
PRINT_PLUGIN_INFO([pipewire],[Pipewire output plugin],[test "x$HAVE_PIPEWIRE_PLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([pulse],[PulseAudio output plugin],[test "x$HAVE_PULSEPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([rgscanner],[plugin for ReplayGain scanner support],[test "x$HAVE_RGSCANNER" = "xyes"])
PRINT_PLUGIN_INFO([sc68],[sc68 Atari ST And Amiga player],[test "x$HAVE_SC68" = "xyes"])
PRINT_PLUGIN_INFO([shellexec],[shell commands plugin],[test "x$HAVE_SHELLEXEC" = "xyes"])
PRINT_PLUGIN_INFO([shellexecui],[GTK user interface for setting up shellexec plugin],[test "x$HAVE_SHELLEXECUI" = "xyes"])
PRINT_PLUGIN_INFO([shn],[SHN plugin based on xmms-shn],[test "x$HAVE_SHN" = "xyes"])
PRINT_PLUGIN_INFO([sid],[SID player based on libsidplay2],[test "x$HAVE_SID" = "xyes"])
PRINT_PLUGIN_INFO([sndfile],[PCM (wav,aiff,etc) player based on libsndfile],[test "x$HAVE_SNDFILEPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([soundtouch],[SoundTouch plugin used for time stretching and pitch shifting],[test "x$HAVE_SOUNDTOUCH" = "xyes"])
PRINT_PLUGIN_INFO([stdio],[Standard IO plugin],[true])
PRINT_PLUGIN_INFO([supereq],[Equalizer based on Super EQ library by Naoki Shibata],[test "x$HAVE_SUPEREQ" = "xyes"])
PRINT_PLUGIN_INFO([tta],[TTA player plugin],[test "x$HAVE_TTA" = "xyes"])
PRINT_PLUGIN_INFO([vfs_curl],[http/ftp streaming support],[test "x$HAVE_VFS_CURL" = "xyes"])
PRINT_PLUGIN_INFO([vfs_zip],[zip archive support],[test "x$HAVE_VFS_ZIP" = "xyes"])
PRINT_PLUGIN_INFO([vorbis],[ogg vorbis player],[test "x$HAVE_VORBISPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([vtx],[vtx file player (ay8910/12 emulation)],[test "x$HAVE_VTX" = "xyes"])
PRINT_PLUGIN_INFO([wavpack],[wavpack player],[test "x$HAVE_WAVPACKPLUGIN" = "xyes"])
PRINT_PLUGIN_INFO([wildmidi],[WildMidi player plugin],[test "x$HAVE_WILDMIDI" = "xyes"])
PRINT_PLUGIN_INFO([wma],[WMA plugin],[test "x$HAVE_WMA" = "xyes"])
dnl PRINT_PLUGIN_INFO([statusnotifier],[plugin for KDE tray icon support],[test "x$HAVE_STATUSNOTIFIER" = "xyes"])

echo
