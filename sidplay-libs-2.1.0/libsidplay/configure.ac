dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.am)
AC_CONFIG_AUX_DIR(unix)
AM_CONFIG_HEADER(unix/config.h)

dnl Setup library CURRENT, REVISION and AGE
LIBCUR=1
LIBREV=0
LIBAGE=0
AM_INIT_AUTOMAKE(libsidplay,2.$LIBCUR.$LIBREV)
dnl libtool-style version-info number
LIBVERSION=$LIBCUR:$LIBREV:$LIBAGE
AC_SUBST(LIBVERSION)

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX

dnl Initialize libtool.
AC_LIBTOOL_WIN32_DLL
MY_CONFIG_LIBTOOL

dnl Checks for header files.
AC_HEADER_STDC
dnl Check for AC99 headers
AC_CHECK_HEADERS(stdbool.h)
if test "$ac_cv_header_stdbool_h" = yes; then
    MY_SUBST_DEF(SID_HAVE_STDBOOL_H)
else
    MY_SUBST_UNDEF(SID_HAVE_STDBOOL_H)
fi
AC_CREATE_STDINT_H(unix/sidint.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN
if test $ac_cv_c_bigendian = yes; then
    MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_BIGENDIAN)
else
    MY_SUBST(SID_WORDS_ENDIANESS,SID_WORDS_LITTLEENDIAN)
fi

AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short int, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long int, 4)

MY_SUBST(SID_SIZEOF_CHAR,$ac_cv_sizeof_char)
MY_SUBST(SID_SIZEOF_SHORT_INT,$ac_cv_sizeof_short_int)
MY_SUBST(SID_SIZEOF_INT,$ac_cv_sizeof_int)
MY_SUBST(SID_SIZEOF_LONG_INT,$ac_cv_sizeof_long_int)

dnl Checks for library functions.
AC_CHECK_FUNC(strncasecmp,[AC_DEFINE(HAVE_STRNCASECMP,,
  [Define if you have the strncasecmp function.])],[])
AC_CHECK_FUNC(strcasecmp,[AC_DEFINE(HAVE_STRCASECMP,,
  [Define if you have the strcasecmp function.])],[])
AC_CHECK_HEADER(strstrea.h,[AC_DEFINE(HAVE_STRSTREA_H,,
  [Define if you have the <strstrea.h> header file.])],[])

AC_SYS_LONG_FILE_NAMES

dnl Use C++ for other tests.
AC_LANG_CPLUSPLUS

dnl Check whether the compiler has bool.
MY_CHECK_BOOL
if test "$my_cv_have_bool" = yes; then
    MY_SUBST_DEF(SID_HAVE_BOOL)
else
    MY_SUBST_UNDEF(SID_HAVE_BOOL)
fi

dnl Check for exceptions.
MY_CHECK_EXCEPTIONS

dnl Check ANSI C++ ios features
MY_CHECK_IOS_BIN
MY_CHECK_IOS_OPENMODE

builders=${libdir}/sidplay/builders
AC_ARG_WITH(sidbuilders,
    [  --with-sidbuilders=DIR
        where the sid builder libraries are to be installed],
    [builders="$withval"]
)
AC_SUBST(builders)

dnl Add absolute directory for header files
CXXFLAGS="$CXXFLAGS -DHAVE_UNIX -I\$(top_srcdir)/include \
-I\$(top_srcdir)/include/sidplay"
ACLOCAL_AMFLAGS="-I unix"
AC_SUBST(ACLOCAL_AMFLAGS)

AC_OUTPUT(
Makefile \
include/Makefile \
include/sidplay/Makefile \
src/Makefile \
src/c64/Makefile \
src/mos6510/Makefile \
src/mos6526/Makefile \
src/mos656x/Makefile \
src/sid6526/Makefile \
src/sidtune/Makefile \
src/xsid/Makefile \
unix/Makefile \
unix/libsidplay2.spec \
unix/libsidplay2.pc \
unix/sidconfig.h \
)
