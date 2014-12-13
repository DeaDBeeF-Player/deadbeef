# wavpack 4.60.1 configure.ac

AC_INIT(wavpack, 4.60.1, bryant@wavpack.com)
AM_INIT_AUTOMAKE(wavpack, 4.60.1, bryant@wavpack.com)
AC_CONFIG_SRCDIR([src/pack.c])
AM_MAINTAINER_MODE

LIBWAVPACK_MAJOR=4
LIBWAVPACK_MINOR=60
LIBWAVPACK_MICRO=1
LIBWAVPACK_VERSION_STRING=$LIBWAVPACK_MAJOR.$LIBWAVPACK_MINOR.$LIBWAVPACK_MICRO

AC_DEFINE_UNQUOTED(LIBWAVPACK_MAJOR, $LIBWAVPACK_MAJOR, [libwavpack major version])
AC_DEFINE_UNQUOTED(LIBWAVPACK_MINOR, $LIBWAVPACK_MINOR, [libwavpack minor version])
AC_DEFINE_UNQUOTED(LIBWAVPACK_MICRO, $LIBWAVPACK_MICRO, [libwavpack micro version])
AC_DEFINE_UNQUOTED(LIBWAVPACK_VERSION_STRING, "$LIBWAVPACK_VERSION_STRING", [libwavpack version string])

# set libtool versioning
# +1 :  0 : +1   == new interface that does not break old one.
# +1 :  0 :  0   == changed/removed an interface. Breaks old apps.
#  ? : +1 :  ?   == internal changes that doesn't break anything.
# CURRENT : REVISION : AGE
LT_CURRENT=2
LT_REVISION=4
LT_AGE=1

AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

# Check for os version
VERSION_OS=$(uname -s)
AC_DEFINE_UNQUOTED(VERSION_OS, "$VERSION_OS", [os version])

# Check for processor characteristics
AC_C_BIGENDIAN(AC_DEFINE([HIGHFIRST], [1], [big-endian machine]))

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AM_PROG_CC_C_O

AC_HEADER_STDC

# Checks for libraries.
AC_CHECK_LIBM
AC_SUBST(LIBM)

# Check if cross-compiling on mingw
AC_MINGW32

if test "x$MINGW32" != "xyes" ; then
# Check for iconv
AC_ARG_WITH(iconv, [[  --with-iconv[=DIR]      add ICONV support [yes]]])
if test "$with_iconv" = "no" ; then
    AC_MSG_ERROR([[Sorry, you can't deactivate iconv.]])
else
    if test "$with_iconv" != "yes" -a "$with_iconv" != "" ; then
       ICONV_CFLAGS="${CPPFLAGS} -I$with_iconv/include"
       ICONV_LIBS="-L$with_iconv/lib"
    fi

    AC_CHECK_HEADER(iconv.h,
        AC_MSG_CHECKING(for iconv)
        AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],[
iconv_t cd = iconv_open ("","");
iconv (cd, NULL, NULL, NULL, NULL);],[
            AC_MSG_RESULT(yes)
            WITH_ICONV=1],[
            AC_MSG_RESULT(no)
            AC_MSG_CHECKING(for iconv in -liconv)

            old_ldflags="${LDFLAGS}"
            old_libs="${LIBS}"
            LDFLAGS="${LDFLAGS} ${ICONV_LIBS}"
            LIBS="${LIBS} -liconv"

            AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],[
iconv_t cd = iconv_open ("","");
iconv (cd, NULL, NULL, NULL, NULL);],[
                AC_MSG_RESULT(yes)
                WITH_ICONV=1
                ICONV_LIBS="${ICONV_LIBS} -liconv"
                ICONV="-liconv"],[
                AC_MSG_ERROR([[Can't find iconv libraries.]])])
            LDFLAGS=$old_ldflags
            LIBS=$old_libs
            ]),
            AC_MSG_ERROR([[Can't find iconv headers.]]))
fi
AC_SUBST(ICONV_LIBS)
AC_SUBST(ICONV)
fi

AC_ARG_ENABLE(mmx, [[  --enable-mmx            enable MMX optimizations [default=no]]])
if test "x$enable_mmx" = "xyes" ; then
    AC_DEFINE([OPT_MMX], 1, [MMX optimization])
    $CC $CFLAGS -mmmx -S -o /dev/null -xc /dev/null > /dev/null 2>&1 && CFLAGS="-mmmx $CFLAGS"
fi

AC_ARG_ENABLE(man,
              [AC_HELP_STRING([--enable-man],
                              [regenerate man pages from Docbook [default=no]])],enable_man=yes,
              enable_man=no)

if test "${enable_man}" != no; then
  dnl
  dnl Check for xsltproc
  dnl
  AC_PATH_PROG([XSLTPROC], [xsltproc])
  if test -z "$XSLTPROC"; then
    enable_man=no
  fi

  dnl check for DocBook DTD and stylesheets in the local catalog.
  JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
     [DocBook XML DTD V4.1.2],,enable_man=no)
  JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
     [DocBook XSL Stylesheets],,enable_man=no)
fi

AM_CONDITIONAL(ENABLE_MAN, test x$enable_man != xno)

AC_CONFIG_FILES(
Makefile
wavpack.pc
src/Makefile
include/Makefile
cli/Makefile
man/Makefile)
AC_OUTPUT
