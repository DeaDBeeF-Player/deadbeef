dnl Process this file with autoconf to produce a configure script.
AC_INIT(Makefile.am)
AC_CONFIG_AUX_DIR(unix)
AM_CONFIG_HEADER(unix/config.h)

dnl Setup library CURRENT, REVISION and AGE
LIBCUR=0
LIBREV=0
LIBAGE=0
AM_INIT_AUTOMAKE(resid-builder,1.$LIBCUR.$LIBREV)
dnl libtool-style version-info number
LIBVERSION=$LIBCUR:$LIBREV:$LIBAGE
AC_SUBST(LIBVERSION)

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX

dnl Initialize libtool.
AC_DISABLE_SHARED
AC_LIBTOOL_WIN32_DLL
MY_CONFIG_LIBTOOL

dnl Use C++ for tests.
AC_LANG_CPLUSPLUS

dnl Check for nothrow allocator.
MY_CHECK_EXCEPTIONS

dnl Checks for libraries.
SID2_LIB_CHECKS
SID2_FIND_LIBRESID
SID2_FIND_LIBSIDPLAY2(prefix)

AC_MSG_CHECKING([for sidbuilders install directory])

dnl setup builder dir
test "$prefix" = NONE && prefix="$LIBSIDPLAY2_PREFIX"
test "$libdir" = '${exec_prefix}/lib' && libdir="$LIBSIDPLAY2_BUILDERS"

AC_MSG_RESULT([${libdir}])

dnl Add absolute directory for header files
CXXFLAGS="$CXXFLAGS -I\$(top_srcdir)/include \
-I\$(top_srcdir)/include/sidplay/builders \
\$(LIBRESID_CXXFLAGS) \$(LIBSIDPLAY2_CXXFLAGS)"

ACLOCAL_AMFLAGS="-I unix"
AC_SUBST(ACLOCAL_AMFLAGS)

AC_OUTPUT(
Makefile \
include/Makefile \
include/sidplay/Makefile \
include/sidplay/builders/Makefile \
src/Makefile \
unix/Makefile \
)
