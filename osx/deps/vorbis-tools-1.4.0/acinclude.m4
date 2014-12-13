# Configure paths for libogg
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl XIPH_PATH_OGG([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libogg, and define OGG_CFLAGS and OGG_LIBS
dnl
AC_DEFUN([XIPH_PATH_OGG],
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ogg,AC_HELP_STRING([--with-ogg=PFX],[Prefix where libogg is installed (optional)]), ogg_prefix="$withval", ogg_prefix="")
AC_ARG_WITH(ogg-libraries,AC_HELP_STRING([--with-ogg-libraries=DIR],[Directory where libogg library is installed (optional)]), ogg_libraries="$withval", ogg_libraries="")
AC_ARG_WITH(ogg-includes,AC_HELP_STRING([--with-ogg-includes=DIR],[Directory where libogg header files are installed (optional)]), ogg_includes="$withval", ogg_includes="")
AC_ARG_ENABLE(oggtest,AC_HELP_STRING([--disable-oggtest],[Do not try to compile and run a test Ogg program]),, enable_oggtest=yes)

  if test "x$ogg_libraries" != "x" ; then
    OGG_LIBS="-L$ogg_libraries"
  elif test "x$ogg_prefix" = "xno" || test "x$ogg_prefix" = "xyes" ; then
    OGG_LIBS=""
  elif test "x$ogg_prefix" != "x" ; then
    OGG_LIBS="-L$ogg_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    OGG_LIBS="-L$prefix/lib"
  fi

  if test "x$ogg_prefix" != "xno" ; then
    OGG_LIBS="$OGG_LIBS -logg"
  fi

  if test "x$ogg_includes" != "x" ; then
    OGG_CFLAGS="-I$ogg_includes"
  elif test "x$ogg_prefix" = "xno" || test "x$ogg_prefix" = "xyes" ; then
    OGG_CFLAGS=""
  elif test "x$ogg_prefix" != "x" ; then
    OGG_CFLAGS="-I$ogg_prefix/include"
  elif test "x$prefix" != "xNONE"; then
    OGG_CFLAGS="-I$prefix/include"
  fi

  AC_MSG_CHECKING(for Ogg)
  if test "x$ogg_prefix" = "xno" ; then
    no_ogg="disabled"
    enable_oggtest="no"
  else
    no_ogg=""
  fi


  if test "x$enable_oggtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $OGG_CFLAGS"
    LIBS="$LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Ogg is sufficiently new.
dnl
      rm -f conf.oggtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

int main ()
{
  system("touch conf.oggtest");
  return 0;
}

],, no_ogg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ogg" = "xdisabled" ; then
     AC_MSG_RESULT(no)
     ifelse([$2], , :, [$2])
  elif test "x$no_ogg" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])
  else
     AC_MSG_RESULT(no)
     if test -f conf.oggtest ; then
       :
     else
       echo "*** Could not run Ogg test program, checking why..."
       CFLAGS="$CFLAGS $OGG_CFLAGS"
       LIBS="$LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ogg/ogg.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Ogg or finding the wrong"
       echo "*** version of Ogg. If it is not finding Ogg, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Ogg was incorrectly installed"
       echo "*** or that you have moved Ogg since it was installed. In the latter case, you"
       echo "*** may want to edit the ogg-config script: $OGG_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     OGG_CFLAGS=""
     OGG_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(OGG_CFLAGS)
  AC_SUBST(OGG_LIBS)
  rm -f conf.oggtest
])

# Configure paths for libvorbis
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl XIPH_PATH_VORBIS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libvorbis, and define VORBIS_CFLAGS and VORBIS_LIBS
dnl
AC_DEFUN([XIPH_PATH_VORBIS],
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(vorbis,AC_HELP_STRING([--with-vorbis=PFX],[Prefix where libvorbis is installed (optional)]), vorbis_prefix="$withval", vorbis_prefix="")
AC_ARG_WITH(vorbis-libraries,AC_HELP_STRING([--with-vorbis-libraries=DIR],[Directory where libvorbis library is installed (optional)]), vorbis_libraries="$withval", vorbis_libraries="")
AC_ARG_WITH(vorbis-includes,AC_HELP_STRING([--with-vorbis-includes=DIR],[Directory where libvorbis header files are installed (optional)]), vorbis_includes="$withval", vorbis_includes="")
AC_ARG_ENABLE(vorbistest,AC_HELP_STRING([--disable-vorbistest],[Do not try to compile and run a test Vorbis program]),, enable_vorbistest=yes)

  if test "x$vorbis_libraries" != "x" ; then
    VORBIS_LIBS="-L$vorbis_libraries"
  elif test "x$vorbis_prefix" = "xno" || test "x$vorbis_prefix" = "xyes" ; then
    VORBIS_LIBS=""
  elif test "x$vorbis_prefix" != "x" ; then
    VORBIS_LIBS="-L$vorbis_prefix/lib"
  elif test "x$prefix" != "xNONE"; then
    VORBIS_LIBS="-L$prefix/lib"
  fi

  VORBISFILE_LIBS="$VORBIS_LIBS -lvorbisfile"
  VORBISENC_LIBS="$VORBIS_LIBS -lvorbisenc"
  if test "x$vorbis_prefix" != "xno" ; then
    VORBIS_LIBS="$VORBIS_LIBS -lvorbis"
  fi

  if test "x$vorbis_includes" != "x" ; then
    VORBIS_CFLAGS="-I$vorbis_includes"
  elif test "x$vorbis_prefix" = "xno" || test "x$vorbis_prefix" = "xyes" ; then
    VORBIS_CFLAGS=""
  elif test "x$vorbis_prefix" != "x" ; then
    VORBIS_CFLAGS="-I$vorbis_prefix/include"
  elif test "x$prefix" != "xNONE"; then
    VORBIS_CFLAGS="-I$prefix/include"
  fi

  xiph_saved_LIBS="$LIBS"
  LIBS=""
  AC_SEARCH_LIBS(cos, m, [VORBIS_LIBS="$VORBIS_LIBS $LIBS"], [
    AC_MSG_WARN([cos() not found in math library, vorbis installation may not work])
  ])
  LIBS="$xiph_saved_LIBS"

  AC_MSG_CHECKING(for Vorbis)
  if test "x$vorbis_prefix" = "xno" ; then
    no_vorbis="disabled"
    enable_vorbistest="no"
  else
    no_vorbis=""
  fi


  if test "x$enable_vorbistest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $VORBIS_CFLAGS $OGG_CFLAGS"
    LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Vorbis is sufficiently new.
dnl
      rm -f conf.vorbistest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

int main ()
{
  system("touch conf.vorbistest");
  return 0;
}

],, no_vorbis=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_vorbis" = "xdisabled" ; then
     AC_MSG_RESULT(no)
     ifelse([$2], , :, [$2])
  elif test "x$no_vorbis" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])
  else
     AC_MSG_RESULT(no)
     if test -f conf.vorbistest ; then
       :
     else
       echo "*** Could not run Vorbis test program, checking why..."
       CFLAGS="$CFLAGS $VORBIS_CFLAGS"
       LIBS="$LIBS $VORBIS_LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <vorbis/codec.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Vorbis or finding the wrong"
       echo "*** version of Vorbis. If it is not finding Vorbis, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Vorbis was incorrectly installed"
       echo "*** or that you have moved Vorbis since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     VORBIS_CFLAGS=""
     VORBIS_LIBS=""
     VORBISFILE_LIBS=""
     VORBISENC_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(VORBIS_CFLAGS)
  AC_SUBST(VORBIS_LIBS)
  AC_SUBST(VORBISFILE_LIBS)
  AC_SUBST(VORBISENC_LIBS)
  rm -f conf.vorbistest
])

# ao.m4
# Configure paths for libao
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl XIPH_PATH_AO([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libao, and define AO_CFLAGS and AO_LIBS
dnl
AC_DEFUN([XIPH_PATH_AO],
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ao,AC_HELP_STRING([--with-ao=PFX],[Prefix where libao is installed (optional)]), ao_prefix="$withval", ao_prefix="")
AC_ARG_WITH(ao-libraries,AC_HELP_STRING([--with-ao-libraries=DIR],[Directory where libao library is installed (optional)]), ao_libraries="$withval", ao_libraries="")
AC_ARG_WITH(ao-includes,AC_HELP_STRING([--with-ao-includes=DIR],[Directory where libao header files are installed (optional)]), ao_includes="$withval", ao_includes="")
AC_ARG_ENABLE(aotest,AC_HELP_STRING([--disable-aotest],[Do not try to compile and run a test ao program]),, enable_aotest=yes)

  if test "x$ao_prefix" != "xno"
  then
    if test "x$ao_libraries" != "x" ; then
      AO_LIBS="-L$ao_libraries"
  elif test "x$ao_prefix" = "xno" || test "x$ao_prefix" = "xyes" ; then
    AO_LIBS=""
    elif test "x$ao_prefix" != "x" -a "x$ao_prefix" != "xyes"; then
      AO_LIBS="-L$ao_prefix/lib"
    elif test "x$prefix" != "xNONE"; then
      AO_LIBS="-L$prefix/lib"
    fi

    if test "x$ao_includes" != "x" ; then
      AO_CFLAGS="-I$ao_includes"
  elif test "x$ao_prefix" = "xno" || test "x$ao_prefix" = "xyes" ; then
    AO_CFLAGS=""
    elif test "x$ao_prefix" != "x" -a "x$ao_prefix" != "xyes"; then
      AO_CFLAGS="-I$ao_prefix/include"
    elif test "x$prefix" != "xNONE"; then
      AO_CFLAGS="-I$prefix/include"
    fi

    # see where dl* and friends live
    AC_CHECK_FUNCS(dlopen, [AO_DL_LIBS=""], [
      AC_CHECK_LIB(dl, dlopen, [AO_DL_LIBS="-ldl"], [
        AC_MSG_WARN([could not find dlopen() needed by libao sound drivers
        your system may not be supported.])
      ])
    ])

  if test "x$ao_prefix" != "xno" ; then
    AO_LIBS="$AO_LIBS -lao $AO_DL_LIBS"
  fi

    AC_MSG_CHECKING(for ao)
  if test "x$ao_prefix" = "xno" ; then
    no_ao="disabled"
    enable_aotest="no"
  else
    no_ao=""
  fi


    if test "x$enable_aotest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $AO_CFLAGS"
      LIBS="$LIBS $AO_LIBS"
dnl
dnl Now check if the installed ao is sufficiently new.
dnl
      rm -f conf.aotest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ao/ao.h>

int main ()
{
  system("touch conf.aotest");
  return 0;
}

],, no_ao=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
    fi

    if test "x$no_ao" = "xdisabled" ; then
       AC_MSG_RESULT(no)
       ifelse([$2], , :, [$2])
    elif test "x$no_ao" = "x" ; then
       AC_MSG_RESULT(yes)
       ifelse([$1], , :, [$1])
    else
       AC_MSG_RESULT(no)
       if test -f conf.aotest ; then
         :
       else
         echo "*** Could not run ao test program, checking why..."
         CFLAGS="$CFLAGS $AO_CFLAGS"
         LIBS="$LIBS $AO_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ao/ao.h>
],       [ return 0; ],
         [ echo "*** The test program compiled, but did not run. This usually means"
         echo "*** that the run-time linker is not finding ao or finding the wrong"
         echo "*** version of ao. If it is not finding ao, you'll need to set your"
         echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
         echo "*** to the installed location  Also, make sure you have run ldconfig if that"
         echo "*** is required on your system"
         echo "***"
         echo "*** If you have an old version installed, it is best to remove it, although"
         echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
         [ echo "*** The test program failed to compile or link. See the file config.log for the"
         echo "*** exact error that occured. This usually means ao was incorrectly installed"
         echo "*** or that you have moved ao since it was installed." ])
         CFLAGS="$ac_save_CFLAGS"
         LIBS="$ac_save_LIBS"
       fi
       AO_CFLAGS=""
       AO_LIBS=""
       ifelse([$2], , :, [$2])
    fi
    AC_SUBST(AO_CFLAGS)
    AC_SUBST(AO_LIBS)
    rm -f conf.aotest
  fi
])

dnl This macros shamelessly stolen from
dnl http://gcc.gnu.org/ml/gcc-bugs/2001-06/msg01398.html.
dnl Written by Bruno Haible.

AC_DEFUN([AM_ICONV],
[
  dnl Some systems have iconv in libc, some have it in libiconv (OSF/1 and
  dnl those with the standalone portable GNU libiconv installed).

  AC_ARG_WITH([libiconv-prefix],
[  --with-libiconv-prefix=DIR  search for libiconv in DIR/include and DIR/lib], [
    for dir in `echo "$withval" | tr : ' '`; do
      if test -d $dir/include; then CPPFLAGS="$CPPFLAGS -I$dir/include"; fi
      if test -d $dir/lib; then LDFLAGS="$LDFLAGS -L$dir/lib"; fi
    done
   ])

  AC_CACHE_CHECK(for iconv, am_cv_func_iconv, [
    am_cv_func_iconv="no, consider installing GNU libiconv"
    am_cv_lib_iconv=no
    AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
      [iconv_t cd = iconv_open("","");
       iconv(cd,NULL,NULL,NULL,NULL);
       iconv_close(cd);],
      am_cv_func_iconv=yes)
    if test "$am_cv_func_iconv" != yes; then
      am_save_LIBS="$LIBS"
      LIBS="$LIBS -liconv"
      AC_TRY_LINK([#include <stdlib.h>
#include <iconv.h>],
        [iconv_t cd = iconv_open("","");
         iconv(cd,NULL,NULL,NULL,NULL);
         iconv_close(cd);],
        am_cv_lib_iconv=yes
        am_cv_func_iconv=yes)
      LIBS="$am_save_LIBS"
    fi
  ])
  if test "$am_cv_func_iconv" = yes; then
    AC_DEFINE(HAVE_ICONV, 1, [Define if you have the iconv() function.])
    AC_MSG_CHECKING([for iconv declaration])
    AC_CACHE_VAL(am_cv_proto_iconv, [
      AC_TRY_COMPILE([
#include <stdlib.h>
#include <iconv.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
size_t iconv (iconv_t cd, char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);
#else
size_t iconv();
#endif
], [], am_cv_proto_iconv_arg1="", am_cv_proto_iconv_arg1="const")
      am_cv_proto_iconv="extern size_t iconv (iconv_t cd, $am_cv_proto_iconv_arg1 char * *inbuf, size_t *inbytesleft, char * *outbuf, size_t *outbytesleft);"])
    am_cv_proto_iconv=`echo "[$]am_cv_proto_iconv" | tr -s ' ' | sed -e 's/( /(/'`
    AC_MSG_RESULT([$]{ac_t:-
         }[$]am_cv_proto_iconv)
    AC_DEFINE_UNQUOTED(ICONV_CONST, $am_cv_proto_iconv_arg1,
      [Define as const if the declaration of iconv() needs const.])
  fi
  LIBICONV=
  if test "$am_cv_lib_iconv" = yes; then
    LIBICONV="-liconv"
  fi
  AC_SUBST(LIBICONV)
])

dnl From Bruno Haible.
dnl
AC_DEFUN([AM_LANGINFO_CODESET],
[
  AC_CACHE_CHECK([for nl_langinfo and CODESET], am_cv_langinfo_codeset,
    [AC_TRY_LINK([#include <langinfo.h>],
      [char* cs = nl_langinfo(CODESET);],
      am_cv_langinfo_codeset=yes,
      am_cv_langinfo_codeset=no)
    ])
  if test $am_cv_langinfo_codeset = yes; then
    AC_DEFINE(HAVE_LANGINFO_CODESET, 1,
      [Define if you have <langinfo.h> and nl_langinfo(CODESET).])
  fi
])

dnl AM_PATH_CURL([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libcurl, and define CURL_CFLAGS and CURL_LIBS
dnl
AC_DEFUN([AM_PATH_CURL],
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(curl,[  --with-curl=PFX   Prefix where libcurl is installed (optional)], curl_prefix="$withval", curl_prefix="")
AC_ARG_WITH(curl-libraries,[  --with-curl-libraries=DIR   Directory where libcurl library is installed (optional)], curl_libraries="$withval", curl_libraries="")
AC_ARG_WITH(curl-includes,[  --with-curl-includes=DIR   Directory where libcurl header files are installed (optional)], curl_includes="$withval", curl_includes="")
AC_ARG_ENABLE(curltest, [  --disable-curltest       Do not try to compile and run a test libcurl program],, enable_curltest=yes)

if test "x$curl_prefix" != "xno" ; then

  if test "x$curl_libraries" != "x" ; then
    CURL_LIBS="-L$curl_libraries"
  elif test "x$curl_prefix" != "x" ; then
    CURL_LIBS="-L$curl_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    CURL_LIBS="-L$prefix/lib"
  fi

  CURL_LIBS="$CURL_LIBS -lcurl"

  if test "x$curl_includes" != "x" ; then
    CURL_CFLAGS="-I$curl_includes"
  elif test "x$curl_prefix" != "x" ; then
    CURL_CFLAGS="-I$curl_prefix/include"
  elif test "x$prefix" != "xNONE"; then
    CURL_CFLAGS="-I$prefix/include"
  fi

  AC_MSG_CHECKING(for libcurl)
  no_curl=""


  if test "x$enable_curltest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $CURL_CFLAGS"
    LIBS="$LIBS $CURL_LIBS"
dnl
dnl Now check if the installed libcurl is sufficiently new.
dnl
      rm -f conf.curltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int main ()
{
  system("touch conf.curltest");
  return 0;
}

],, no_curl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_curl" = "x" ; then
     AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_CURL, 1, [Define if you have libcurl.])
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.curltest ; then
       :
     else
       echo "*** Could not run libcurl test program, checking why..."
       CFLAGS="$CFLAGS $CURL_CFLAGS"
       LIBS="$LIBS $CURL_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <curl/curl.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding libcurl or finding the wrong"
       echo "*** version of libcurl. If it is not finding libcurl, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means libcurl was incorrectly installed"
       echo "*** or that you have moved libcurl since it was installed." ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     CURL_CFLAGS=""
     CURL_LIBS=""
     ifelse([$2], , :, [$2])
  fi
else
  CURL_CFLAGS=""
  CURL_LIBS=""
fi
  AC_SUBST(CURL_CFLAGS)
  AC_SUBST(CURL_LIBS)
  rm -f conf.curltest
])


dnl ACX_PTHREAD macro by Steven G. Johnson <stevenj@alum.mit.edu> and
dnl Alejandro Forero Cuervo <bachue@bachue.com>.  Found at:
dnl http://www.gnu.org/software/ac-archive/Installed_Packages/acx_pthread.html

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
acx_pthread_ok=no

# First, check if the POSIX threads header, pthread.h, is available.
# If it isn't, don't bother looking for the threads libraries.
AC_CHECK_HEADER(pthread.h, , acx_pthread_ok=noheader)

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all.

acx_pthread_flags="pthreads none -Kthread -kthread lthread pthread -pthread -pthreads -mthreads --thread-safe -mt"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:
        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0);
                     pthread_cancel(0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: threads are created detached by default
        # and the JOINABLE attribute has a nonstandard name (UNDETACHED).
        AC_MSG_CHECKING([for joinable pthread attribute])
        AC_TRY_LINK([#include <pthread.h>],
                    [int attr=PTHREAD_CREATE_JOINABLE;],
                    ok=PTHREAD_CREATE_JOINABLE, ok=unknown)
        if test x"$ok" = xunknown; then
                AC_TRY_LINK([#include <pthread.h>],
                            [int attr=PTHREAD_CREATE_UNDETACHED;],
                            ok=PTHREAD_CREATE_UNDETACHED, ok=unknown)
        fi
        if test x"$ok" != xPTHREAD_CREATE_JOINABLE; then
                AC_DEFINE(PTHREAD_CREATE_JOINABLE, $ok,
                          [Define to the necessary symbol if this constant
                           uses a non-standard name on your system.])
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xunknown; then
                AC_MSG_WARN([we do not know how to create joinable pthreads])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd*)     flag="-D_THREAD_SAFE";;
                *solaris* | alpha*-osf*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi

])
