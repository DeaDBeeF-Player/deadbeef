dnl Process this file with autoconf to produce a configure script

dnl ------------------------------------------------
dnl Initialization
dnl ------------------------------------------------

AC_INIT(oggenc/encode.c)

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_PREREQ(2.53)

AM_INIT_AUTOMAKE([vorbis-tools],[1.4.0])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER([config.h])

AC_USE_SYSTEM_EXTENSIONS

dnl --------------------------------------------------  
dnl Check for programs
dnl --------------------------------------------------  

dnl save $CFLAGS since AC_PROG_CC likes to insert "-g -O2"
dnl if $CFLAGS is blank
cflags_save="$CFLAGS"
AC_PROG_CC
CFLAGS="$cflags_save"

AC_PROG_LIBTOOL

ALL_LINGUAS="be cs da en_GB eo es fr hr hu nl pl ro ru sk sv uk vi"
AM_GNU_GETTEXT

dnl --------------------------------------------------
dnl System checks
dnl --------------------------------------------------

AC_SYS_LARGEFILE
AC_C_BIGENDIAN

dnl --------------------------------------------------
dnl Set build flags based on environment
dnl --------------------------------------------------

cflags_save="$CFLAGS"
if test -z "$GCC"; then
        case $host in
        *-*-irix*)
                DEBUG="-g -signed"
                CFLAGS="-O2 -w -signed"
                PROFILE="-p -g3 -O2 -signed"
                ;;
        sparc-sun-solaris*)
                DEBUG="-v -g"
                CFLAGS="-xO4 -fast -w -fsimple -native -xcg92"
                PROFILE="-v -xpg -g -xO4 -fast -native -fsimple -xcg92 -Dsuncc"
                ;;
        *)
                DEBUG="-g"
                CFLAGS="-O"
                PROFILE="-g -p"
                ;;
        esac
else
        case $host in
        *-*-linux*)
                DEBUG="-g -Wall -fsigned-char"
                CFLAGS="-O2 -Wall -ffast-math -fsigned-char"
                PROFILE="-Wall -W -pg -g -O2 -ffast-math -fsigned-char"
                ;;
        sparc-sun-*)
                DEBUG="-g -Wall -fsigned-char -mv8"
                CFLAGS="-O20 -ffast-math -fsigned-char -mv8"
                PROFILE="-pg -g -O20 -fsigned-char -mv8"
                ;;
        *-*-darwin*)
                DEBUG="-fno-common -g -Wall -fsigned-char"
                CFLAGS="-fno-common -O4 -Wall -fsigned-char -ffast-math"
                PROFILE="-fno-common -O4 -Wall -pg -g -fsigned-char -ffast-math"
                ;;
        *)
                DEBUG="-g -Wall -fsigned-char"
                CFLAGS="-O2 -fsigned-char"
                PROFILE="-O2 -g -pg -fsigned-char"
                ;;
        esac
fi
CFLAGS="$CFLAGS $cflags_save"
DEBUG="$DEBUG $cflags_save"
PROFILE="$PROFILE $cflags_save"

dnl --------------------------------------------------
dnl Allow tools to be selectively built
dnl --------------------------------------------------
AC_ARG_ENABLE(ogg123, [  --disable-ogg123   Skip building ogg123], build_ogg123="$enableval", build_ogg123="yes")
AC_ARG_ENABLE(oggdec, [  --disable-oggdec   Skip building oggdec], build_oggdec="$enableval", build_oggdec="yes")
AC_ARG_ENABLE(oggenc, [  --disable-oggenc   Skip building oggenc], build_oggenc="$enableval", build_oggenc="yes")
AC_ARG_ENABLE(ogginfo,[  --disable-ogginfo  Skip building ogginfo], build_ogginfo="$enableval", build_ogginfo="yes")
AC_ARG_ENABLE(vcut,   [  --disable-vcut     Skip building vcut], build_vcut="$enableval", build_vcut="yes")
AC_ARG_ENABLE(vorbiscomment, [  --disable-vorbiscomment   Skip building vorbiscomment], build_vorbiscomment="$enableval", build_vorbiscomment="yes")
AC_ARG_WITH(flac,     [  --without-flac     Do not compile FLAC support], build_flac="$withval", build_flac="yes")
AC_ARG_WITH(speex,    [  --without-speex    Do not compile Speex support], build_speex="$withval", build_speex="yes")
AC_ARG_WITH(kate,     [  --without-kate     Do not compile Kate support], build_kate="$withval", build_kate="yes")

dnl --------------------------------------------------
dnl Check for generally needed libraries
dnl --------------------------------------------------

HAVE_OGG=no
dnl first check through pkg-config
dnl check for pkg-config itself so we don't try the m4 macro without pkg-config
AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(OGG, ogg >= 1.0, HAVE_OGG=yes, HAVE_OGG=no)
fi
if test "x$HAVE_OGG" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_OGG(,AC_MSG_ERROR(Ogg needed!))
  libs_save=$LIBS
  LIBS="$OGG_LIBS $VORBIS_LIBS"
  AC_CHECK_FUNC(oggpack_writealign, , AC_MSG_ERROR(Ogg >= 1.0 required !))
  LIBS=$libs_save
fi

dnl check for Vorbis
HAVE_VORBIS=no

dnl first check through pkg-config since it's more flexible

if test "x$HAVE_PKG_CONFIG" = "xyes"
then
  PKG_CHECK_MODULES(VORBIS, vorbis >= 1.3.0, HAVE_VORBIS=yes, HAVE_VORBIS=no)
  dnl also set VORBISENC_LIBS since an examples needs it
  dnl the old .m4 sets this to a value to use on top of VORBIS_LIBS,
  dnl so we do the same here.
  VORBISENC_LIBS="-lvorbisenc"
  VORBISFILE_LIBS="-lvorbisfile"
  AC_SUBST(VORBISENC_LIBS)
  AC_SUBST(VORBISFILE_LIBS)
  libs_save=$LIBS
  LIBS="$OGG_LIBS $VORBIS_LIBS $VORBISFILE_LIBS"
  AC_CHECK_FUNC(ov_read_filter, have_ov_read_filter=yes, have_ov_read_filter=no)
  LIBS=$libs_save
  if test "x$have_ov_read_filter" = "xyes"
  then
    AC_DEFINE(HAVE_OV_READ_FILTER,1,[Defined if we have ov_read_filter()])
  fi
fi
if test "x$HAVE_VORBIS" = "xno"
then
  dnl fall back to the old school test
  XIPH_PATH_VORBIS(,AC_MSG_ERROR(Vorbis needed!))
  AC_CHECK_DECL(OV_ECTL_COUPLING_SET, , , [#include <vorbis/vorbisenc.h>])
  if test "x$ac_cv_have_decl_OV_ECTL_COUPLING_SET" = "xno"
  then
    AC_MSG_ERROR([Vorbis >= 1.3.0 required !])
    HAVE_VORBIS=no
  fi
fi
AM_CONDITIONAL(HAVE_OV_READ_FILTER, test "x$have_ov_read_filter" = "xyes")


SHARE_LIBS='$(top_builddir)/share/libutf8.a $(top_builddir)/share/libgetopt.a'
SHARE_CFLAGS='-I$(top_srcdir)/include'

I18N_CFLAGS='-I$(top_srcdir)/intl'
I18N_LIBS=$INTLLIBS

SOCKET_LIBS=
AC_CHECK_LIB(socket, socket, SOCKET_LIBS="-lsocket")
AC_CHECK_LIB(network, socket, SOCKET_LIBS="-lnetwork")
AC_CHECK_LIB(nsl, gethostbyname, SOCKET_LIBS="-lnsl $SOCKET_LIBS")


dnl --------------------------------------------------
dnl Check for ogg123 critical libraries and other optional libraries
dnl --------------------------------------------------

dnl curl is an optional dependancy of ogg123
if test "x$HAVE_PKG_CONFIG" = "xyes"; then
  PKG_CHECK_MODULES(CURL, libcurl, HAVE_CURL=yes, HAVE_CURL=no)
  if test "x$HAVE_CURL" = "xno"; then
    AM_PATH_CURL(HAVE_CURL=yes, HAVE_CURL=no; AC_MSG_WARN(libcurl missing))
  fi
else
  AM_PATH_CURL(HAVE_CURL=yes, HAVE_CURL=no; AC_MSG_WARN(libcurl missing))
fi
if test "x$HAVE_CURL" = "xyes"; then
  AC_DEFINE(HAVE_CURL,1,[Defined if we have libcurl])
fi

if test "x$build_ogg123" = xyes; then
  AC_MSG_RESULT([checking for ogg123 requirements])
  PKG_CHECK_MODULES(AO, ao >= 1.0.0,,build_ogg123=no; AC_MSG_WARN(libao too old; >= 1.0.0 required))
  ACX_PTHREAD(,build_ogg123=no; AC_MSG_WARN(POSIX threads missing))
fi

dnl -------------------- FLAC ----------------------

FLAC_LIBS=""
if test "x$build_flac" = xyes; then

  AC_CHECK_LIB(m,log,FLAC_LIBS="-lm")

  dnl First check for libFLAC-1.1.3 or later. As of libFLAC 1.1.3,
  dnl OggFLAC functionality has been rolled into libFLAC rather
  dnl than being in a separate libOggFLAC library.

  AC_CHECK_LIB(FLAC, [FLAC__stream_decoder_init_ogg_stream],
    have_libFLAC=yes, have_libFLAC=no, [$FLAC_LIBS $OGG_LIBS])

  if test "x$have_libFLAC" = xyes; then
    FLAC_LIBS="-lFLAC $FLAC_LIBS $OGG_LIBS"
  else
    dnl Check for libFLAC prior to 1.1.3
    AC_CHECK_LIB(FLAC, [FLAC__stream_decoder_process_single],
      [have_libFLAC=yes; FLAC_LIBS="-lFLAC $FLAC_LIBS"],
      AC_MSG_WARN([libFLAC missing])
      have_libFLAC=no, [$FLAC_LIBS]
    )

    AC_CHECK_LIB(OggFLAC, [OggFLAC__stream_decoder_new],
      [FLAC_LIBS="-lOggFLAC $FLAC_LIBS $OGG_LIBS"],
      AC_MSG_WARN([libOggFLAC missing])
      have_libFLAC=no, [$FLAC_LIBS $OGG_LIBS]
    )
  fi

  AC_CHECK_HEADER(FLAC/stream_decoder.h,,
    AC_MSG_WARN(libFLAC headers missing)
    have_libFLAC=no,[ ])

  if test "x$have_libFLAC" = xyes; then
     AC_DEFINE(HAVE_LIBFLAC, 1, [Defined if we have libFLAC])
  else
     build_flac="no"
     FLAC_LIBS=""
  fi
fi
AM_CONDITIONAL(HAVE_LIBFLAC, test "x$have_libFLAC" = "xyes")
AC_SUBST(FLAC_LIBS)

dnl ------------------- Speex ------------------------

SPEEX_LIBS=""
if test "x$build_speex" = xyes; then
  AC_CHECK_LIB(m,log,SPEEX_LIBS="-lm")
  AC_CHECK_LIB(speex, [speex_decoder_init],
    [have_libspeex=yes; SPEEX_LIBS="-lspeex $SPEEX_LIBS"],
    AC_MSG_WARN(libspeex missing)
    have_libspeex=no, [$SPEEX_LIBS]
  )
  AC_CHECK_HEADER(speex/speex.h,,
    AC_MSG_WARN(libspeex headers missing)
    have_libspeex=no,[ ])

  if test "x$have_libspeex" = xyes; then
    AC_DEFINE(HAVE_LIBSPEEX, 1, [Defined if we have libspeex])
  else
    build_speex="no"
    SPEEX_LIBS=""
  fi
fi
AM_CONDITIONAL(HAVE_LIBSPEEX, test "x$have_libspeex" = "xyes")
AC_SUBST(SPEEX_LIBS)

dnl ------------------- Kate -------------------------

KATE_CFLAGS=""
KATE_LIBS=""
if test "x$build_kate" = xyes; then
  AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes)
  if test "x$HAVE_PKG_CONFIG" = "xyes"
  then
    PKG_CHECK_MODULES(KATE, oggkate, HAVE_KATE=yes, HAVE_KATE=no)
  fi
  if test "x$HAVE_KATE" = "xno"
  then
    dnl fall back to the old school test
    AC_CHECK_LIB(m,log,KATE_LIBS="-lm")
    AC_CHECK_LIB(kate, [kate_decode_init],
      [HAVE_KATE=yes; KATE_LIBS="-lkate $KATE_LIBS $OGG_LIBS"],
      AC_MSG_WARN(libkate missing)
      HAVE_KATE=no, [$KATE_LIBS $OGG_LIBS]
    )
    AC_CHECK_LIB(oggkate, [kate_ogg_decode_headerin],
      [HAVE_KATE=yes; KATE_LIBS="-loggkate $KATE_LIBS $OGG_LIBS"],
      AC_MSG_WARN(libkate missing)
      HAVE_KATE=no, [$KATE_LIBS $OGG_LIBS]
    )
    AC_CHECK_HEADER(kate/kate.h,,
      AC_MSG_WARN(libkate headers missing)
      HAVE_KATE=no,[ ])
    AC_CHECK_HEADER(kate/oggkate.h,,
      AC_MSG_WARN(liboggkate headers missing)
      HAVE_KATE=no,[ ])

  fi
  if test "x$HAVE_KATE" = xyes; then
    AC_DEFINE(HAVE_KATE, 1, [Defined if we have libkate])
  else
    build_kate="no"
    KATE_CFLAGS=""
    KATE_LIBS=""
  fi
fi
AM_CONDITIONAL(HAVE_KATE, test "x$HAVE_KATE" = "xyes")
AC_SUBST(KATE_CFLAGS)
AC_SUBST(KATE_LIBS)

dnl --------------------------------------------------
dnl Check for headers
dnl --------------------------------------------------

AC_CHECK_HEADERS([fcntl.h unistd.h])

dnl --------------------------------------------------
dnl Check for library functions
dnl --------------------------------------------------

AC_FUNC_ALLOCA
AM_ICONV
AC_CHECK_FUNCS(atexit on_exit fcntl select stat chmod alphasort scandir)
AM_LANGINFO_CODESET

dnl --------------------------------------------------
dnl Work around FHS stupidity
dnl --------------------------------------------------

if test -z "$mandir"; then
    if test "$prefix" = "/usr"; then
        MANDIR='$(datadir)/man'
    else
        MANDIR='$(prefix)/man'
    fi
else
    MANDIR=$mandir
fi
AC_SUBST(MANDIR)


dnl --------------------------------------------------
dnl Do substitutions
dnl --------------------------------------------------

# add optional subdirs to the build
OPT_SUBDIRS=""
if test "x$build_ogg123" = xyes; then
  OPT_SUBDIRS="$OPT_SUBDIRS ogg123"
fi
if test "x$build_oggenc" = xyes; then
  OPT_SUBDIRS="$OPT_SUBDIRS oggenc"
fi
if test "x$build_oggdec" = xyes; then
  OPT_SUBDIRS="$OPT_SUBDIRS oggdec"
fi
if test "x$build_ogginfo" = xyes; then
  OPT_SUBDIRS="$OPT_SUBDIRS ogginfo"
fi
if test "x$build_vcut" = xyes; then
  OPT_SUBDIRS="$OPT_SUBDIRS vcut"
fi
if test "x$build_vorbiscomment" = xyes; then
  OPT_SUBDIRS="$OPT_SUBDIRS vorbiscomment"
fi
AC_SUBST(OPT_SUBDIRS)

AC_SUBST(DEBUG)
AC_SUBST(PROFILE)
AC_SUBST(SOCKET_LIBS)
AC_SUBST(SHARE_CFLAGS)
AC_SUBST(SHARE_LIBS)
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)
AC_SUBST(I18N_CFLAGS)
AC_SUBST(I18N_LIBS)

AC_OUTPUT([
Makefile
m4/Makefile
po/Makefile.in
intl/Makefile
include/Makefile
share/Makefile
win32/Makefile
oggdec/Makefile
oggenc/Makefile
oggenc/man/Makefile
ogg123/Makefile
vorbiscomment/Makefile
vcut/Makefile
ogginfo/Makefile
debian/Makefile
])

if test "x$build_oggenc" = xyes -a "x$have_libFLAC" != xyes; then
    AC_MSG_WARN([FLAC and/or OggFLAC libraries or headers missing, oggenc 
will NOT be built with FLAC read support.])
fi

if test "x$build_ogg123" != xyes; then
    AC_MSG_WARN([Prerequisites for ogg123 not met, ogg123 will be skipped.
Please ensure that you have POSIX threads, libao, and (optionally) libcurl
libraries and headers present if you would like to build ogg123.])
else
    if test "x$have_libFLAC" != xyes; then
        AC_MSG_WARN([FLAC and/or OggFLAC libraries or headers missing, ogg123 
will NOT be built with FLAC read support.])
    fi
    if test "x$have_libspeex" != xyes; then
        AC_MSG_WARN([Speex libraries and/or headers missing, ogg123 
will NOT be built with Speex read support.])
    fi
    if test "x$HAVE_CURL" != xyes; then
        AC_MSG_WARN([curl libraries and/or headers missing, ogg123 
will NOT be built with http support.])
    fi
    if test "x$HAVE_KATE" != xyes; then
        AC_MSG_WARN([Kate libraries and/or headers missing, oggenc 
will NOT be built with Kate lyrics support.])
    fi
fi
