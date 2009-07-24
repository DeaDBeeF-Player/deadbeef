dnl -------------------------------------------------------------------------
dnl Try to find resid includes and library.
dnl -------------------------------------------------------------------------

AC_DEFUN(SID2_FIND_LIBRESID,
[
    if test "$SID2_LIB_CHECK" != "0"; then
        MY_FIND_LIB(resid,,,resid/sid.h sid.h,SID *mySID,sid2_header)
    else
        MY_FIND_LIB_NO_CHECK(resid,resid/sid.h sid.h,sid2_header)
    fi

    dnl list exported variables here so end up in makefile
    AC_SUBST(LIBRESID_CXXFLAGS)
    AC_SUBST(LIBRESID_LDFLAGS)

    if test "$sid2_header" = sid.h; then
        AC_DEFINE(HAVE_USER_RESID,,
          [Define this to \#include sid.h instead of resid/resid.h]
        )
    fi

    if test "$SID2_LIB_CHECK" != "0"; then    
        AC_MSG_CHECKING([for extended resid features])
        MY_TRY_COMPILE($LIBRESID_CXXFLAGS,$LIBRESID_LDFLAGS,$sid2_header,SID mySID;
                       mySID.mute(0,true),sid2_works)

        if test "$sid2_works" = NO; then
            AC_MSG_RESULT(no);
            # Found non patched version of resid
            AC_MSG_ERROR([
resid requires patching to function with sidplay2.
Patches are available from http://sidplay2.sourceforge.net
            ])
        else
            AC_MSG_RESULT(yes);
        fi
    fi
])
