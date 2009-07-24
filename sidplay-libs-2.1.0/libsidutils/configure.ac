dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.am)
AC_CONFIG_AUX_DIR(unix)
AM_CONFIG_HEADER(unix/config.h)

dnl Setup library CURRENT, REVISION and AGE
LIBCUR=0
LIBREV=2
LIBAGE=0
AM_INIT_AUTOMAKE(libsidutils,1.$LIBCUR.$LIBREV)
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

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

dnl Use C++ for tests.
AC_LANG_CPLUSPLUS

dnl Check for nothrow allocator.
MY_CHECK_EXCEPTIONS

dnl Find libraries
SID2_LIB_CHECKS
SID2_FIND_LIBSIDPLAY2

dnl Add absolute directory for header files
CXXFLAGS="$CXXFLAGS -I\$(top_srcdir)/include \
-I\$(top_srcdir)/include/sidplay/utils \$(LIBSIDPLAY2_CXXFLAGS)"

ACLOCAL_AMFLAGS="-I unix"
AC_SUBST(ACLOCAL_AMFLAGS)

AC_OUTPUT(
Makefile \
include/Makefile \
include/sidplay/Makefile \
include/sidplay/utils/Makefile \
src/Makefile \
src/ini/Makefile \
src/MD5/Makefile \
unix/Makefile \
unix/libsidutils.spec \
unix/libsidutils.pc \
)
