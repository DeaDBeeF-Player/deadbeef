dnl -------------------------------------------------------------------------
dnl Test for working build
dnl $1 - builder library name (lower case)
dnl $2 - CXXFLAGS
dnl $3 - LDFLAGS
dnl $4 - ldflag (output)
dnl -------------------------------------------------------------------------
AC_DEFUN(SID2_TEST_BUILDER,
[
    AC_MSG_CHECKING([for $1 builder module])

    dnl @FIXME@ Builders should export themselves via the base class only
    dnl This is not supported yet
    case $1 in
        resid) sid2_builder=ReSIDBuilder;;
        hardsid) sid2_builder=HardSIDBuilder;;
    esac

    MY_TRY_COMPILE($2,[$3 -l$1-builder],
                   [sidplay/builders/$1.h],
                   [$sid2_builder *myBuilder;], sid2_builder_works)

    dnl Found builder to define variables to indicate
    dnl it and linker flags
    $4=""
    if test "$sid2_builder_works" = YES; then
        sid2_def="HAVE_`echo $1 | tr [a-z] [A-Z]`_BUILDER"
        AC_DEFINE_UNQUOTED($sid2_def)
        $4="-l$1-builder"
        AC_MSG_RESULT(yes)
    else
        AC_MSG_RESULT(no)
    fi
])


dnl -------------------------------------------------------------------------
dnl Find installed builders
dnl NOTE: Before running this command you must have run
dnl libsidplay2 for the necessary path information
dnl -------------------------------------------------------------------------
AC_DEFUN(SID2_FIND_BUILDERS,
[
    AC_MSG_CHECKING([for sidbuilders install directory])

    AC_ARG_WITH(sidbuilders,
        [  --with-sidbuilders=DIR
            where the sid builder libraries are installed],
        [LIBSIDPLAY2_BUILDERS="$withval"]
    )

    AH_TOP([
/* Define supported builder */
#undef HAVE_RESID_BUILDER
#undef HAVE_HARDSID_BUILDER
    ])

    AC_MSG_RESULT($LIBSIDPLAY2_BUILDERS)
    BUILDERS_LDFLAGS=""
    dnl @FIXME@ detection of builders should be automatic
    for sid2_lib in resid hardsid; do
        dnl builder must be lower case
        sid2_lib=`echo $sid2_lib | tr [A-Z] [a-z]`
        SID2_TEST_BUILDER($sid2_lib,
                          $LIBSIDPLAY2_CXXFLAGS,
                          -L$LIBSIDPLAY2_BUILDERS,
                          sid2_ldflags)

        if test "$sid2_ldflags" != ""; then
            BUILDERS_LDFLAGS="$BUILDERS_LDFLAGS $sid2_ldflags"
        fi
    done

    if test "$BUILDERS_LDFLAGS" = ""; then
        AC_MSG_ERROR([
No builder modules were found in the sidbuilders
install dir.  Please check your installation!
                     ]);
    fi

    BUILDERS_LDFLAGS="-L$LIBSIDPLAY2_BUILDERS $BUILDERS_LDFLAGS"
    AC_SUBST(BUILDERS_LDFLAGS)
])


dnl -------------------------------------------------------------------------
dnl Disable library build checks
dnl -------------------------------------------------------------------------
AC_DEFUN(SID2_LIB_CHECKS,
[
    AC_ARG_ENABLE(library-checks,
    [  --disable-library-checks  do not check for working libraries])
    if test x"${enable_library_checks+set}" = xset; then
        SID2_LIB_CHECK=0
    fi
])


dnl -------------------------------------------------------------------------
dnl Find libsidplay2 library
dnl [$1 - variables]
dnl -------------------------------------------------------------------------
AC_DEFUN(SID2_FIND_LIBSIDPLAY2,
[
    if test "$SID2_LIB_CHECK" != "0"; then
        MY_FIND_PKG_CONFIG_LIB(sidplay2,"2.1.0",builders $1,sidplay/sidplay2.h,
                               sidplay2 *myEngine)
    else
        MY_FIND_LIB_NO_CHECK(sidplay2,sidplay/sidplay2.h)
        LIBSIDPLAY2_CXXFLAGS="$LIBSIDPLAY2_CXXFLAGS -DHAVE_UNIX"
        LIBSIDPLAY2_PREFIX=NONE
    fi

    dnl list exported variables here so end up in makefile
    AC_SUBST(LIBSIDPLAY2_CXXFLAGS)
    AC_SUBST(LIBSIDPLAY2_LDFLAGS)
    AC_SUBST(LIBSIDPLAY2_BUILDERS)
])

dnl -------------------------------------------------------------------------
dnl Find libsidutils library
dnl [$1 - variables]
dnl -------------------------------------------------------------------------
AC_DEFUN(SID2_FIND_LIBSIDUTILS,
[
    if test "$SID2_LIB_CHECK" != "0"; then
        MY_FIND_PKG_CONFIG_LIB(sidutils,"1.0.2",$1,sidplay/utils/SidDatabase.h,
                               SidDatabase *d)
    else
        MY_FIND_LIB_NO_CHECK(sidutils,sidplay/utils/SidDatabase.h)
        LIBSIDPLAY2_CXXFLAGS="$LIBSIDPLAY2_CXXFLAGS -DHAVE_UNIX"
    fi

    dnl list exported variables here so end up in makefile
    AC_SUBST(LIBSIDUTILS_CXXFLAGS)
    AC_SUBST(LIBSIDUTILS_LDFLAGS)
])
