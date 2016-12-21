dnl# -*- mode:sh; sh-basic-offset:2; indent-tabs-mode:nil -*-
dnl# 
dnl# the "configure" script is made from this by running GNU "autoconf"
dnl#
dnl# Configuration script for file68
dnl#
dnl# by Benjamin Gerard <http://sourceforge.net/users/benjihan>
dnl#

# ,----------------------------------------------------------------------.
# | Autotools and Package                                                |
# `----------------------------------------------------------------------'

AC_INIT(
  [file68],
  [3.0.0b],
  [http://sourceforge.net/projects/sc68/],
  [file68],
  [libfile68],
  [http://sc68.atari.org])
AC_CONFIG_SRCDIR([src/file68.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.11 silent-rules dist-xz subdir-objects
                  no-installman no-installinfo])
AC_CONFIG_HEADERS([config.h sc68/file68_features.h])
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([win32-dll])

# ,----------------------------------------------------------------------.
# | sc68                                                                 |
# `----------------------------------------------------------------------'

SC68_PACKAGE([sc68 file and utility library.])
SC68_SYSTEM
SC68_OPTIONS
SC68_TOOLS
SC68_CHECKS

# ,----------------------------------------------------------------------.
# | Optional Features                                                    |
# `----------------------------------------------------------------------'

AC_ARG_ENABLE(
  [file68-data],
  [AS_HELP_STRING([--enable-file68-data],
      [file68 shared data to be installed @<:@default=check@:>@])],
  [],[enable_file68_data=check])

AC_ARG_ENABLE(
  [file],
  [AS_HELP_STRING([--enable-file],
      [FILE stream support @<:@default=check@:>@])],
  [],[enable_file=check])

AC_ARG_ENABLE(
  [fd],
  [AS_HELP_STRING([--enable-fd],
      [file descriptor stream support @<:@default=check@:>@])],
  [],[enable_fd=check])

AC_ARG_ENABLE(
  [mem],
  [AS_HELP_STRING([--enable-mem],
      [memory stream support @<:@default=yes@:>@])],
  [],[enable_mem=yes])

AC_ARG_ENABLE(
  [registry],
  [AS_HELP_STRING([--enable-registry],
      [windows registry support @<:@default=check@:>@])],
  [],[enable_reg=check])

AC_ARG_ENABLE(
  [replay-rom],
  [AS_HELP_STRING([--enable-replay-rom],
      [add built-in replay (~130kB) @<:@default=no@:>@])],
  [],[enable_replay_rom=no])

AC_ARG_ENABLE(
  [scheme],
  [AS_HELP_STRING([--enable-scheme],
      [select schemes/protocols available @<:@default=check@:>@])],
  [],[enable_scheme=no])

# ,----------------------------------------------------------------------.
# | External libraries                                                   |
# `----------------------------------------------------------------------'

PAC_CFLAGS="-I[$]includedir"
PAC_LFLAGS="-L[$]libdir -lfile68"

SC68_WITH([unice68])
SC68_WITH([z],[no],[gzdopen],[zlib.h],[zlib68])
SC68_WITH([ao],[no],[ao_initialize],[ao/ao.h],[ao68])
SC68_WITH([curl],[no],[],[curl/curl.h],[curl68])

# Protocols/schemes
# -----------------
if test X"[$]has_curl" = Xno; then
    enable_scheme='n/a'
else
  if test X"[$]enable_scheme" != check; then
    curl_protocols='FILE HTTP'
    if test X"[$]curl_org" = Xpkg-config; then
        eval curl_protocols=`[$]pkgconfig libcurl --variable=supported_protocols`
    fi
    enable_scheme=''; CURL_SCHEMES=''
    for protocol in $curl_protocols; do
      case [$]protocol in
        FILE | *FTP* | HTTP*)
          enable_scheme="[$]enable_scheme[$]protocol "
          CURL_SCHEMES="[$]CURL_SCHEMES[$]protocol\0"
          ;;
      esac
    done
  fi
  AC_DEFINE_UNQUOTED([CURL_SCHEMES],["[$]CURL_SCHEMES]",[Enabled curl schemes])
fi

# Registry68 support
# ------------------
if test X"[$]enable_reg" = Xcheck; then
  if test X"[$]ac_sys_native_win32" = Xyes; then
    AC_CHECK_HEADERS([windows.h],
      [AC_CHECK_HEADERS([winreg.h],
          [enable_reg=yes],[],
          [#include <windows.h>])])
  fi
fi
if test X"[$]enable_reg" = Xyes; then
  AC_DEFINE([USE_REGISTRY68],[1],[Using Windows registry])
else
  enable_reg="no"
fi

# FILE stream support
#--------------------
if test X"[$]enable_file" = Xcheck; then
  AC_CHECK_HEADERS([stdio.h],[enable_file=yes],[enable_file=no])
fi
if test X[$]enable_file = Xno; then
  AC_DEFINE([ISTREAM68_NO_FILE],[1],[Disable FILE* stream support])
fi

# FD stream support
#------------------
if test X"[$]enable_fd" = Xcheck; then
  AC_CHECK_HEADERS([unistd.h],[enable_fd=yes],[enable_fd=no])
fi
if test X[$]enable_fd = Xno; then
  AC_DEFINE([ISTREAM68_NO_FD],[1],[Disable file decriptor stream support])
fi

# MEM stream support
#-------------------
if test X[$]enable_mem = Xno; then
  AC_DEFINE([ISTREAM68_NO_MEM],[1],[Disable memory stream support])
fi

# DATA for file68
#----------------
sc68datadir='$(datadir)/sc68'
AC_SUBST([sc68datadir])

case X"$enable_file68_data" in
  Xno | Xyes )
    ;;
  Xcheck)
    if test X[$]ac_sys_native_win32 = Xyes; then
      enable_file68_data=no
    else
      enable_file68_data=yes
    fi
    ;;
  *)
    AC_MSG_ERROR([invalid value for --enable-file68-data])
    ;;
esac
AC_MSG_CHECKING([whether file68 data to be installed])
AM_CONDITIONAL([FILE68_DATA],[test X"$enable_file68_data" = Xyes])
AC_MSG_RESULT([$enable_file68_data ($sc68datadir)])

# Built-in replay rom
#--------------------
AC_MSG_CHECKING([whether external replays are built-in])
AM_CONDITIONAL([REPLAY68],[test x"$enable_replay_rom" = xyes])
if test x"$enable_replay_rom" = xyes; then
  AC_DEFINE([USE_REPLAY68],[1],[Built-in replays])
else
  enable_replay_rom=no
fi
AC_MSG_RESULT([$enable_replay_rom])

# ,----------------------------------------------------------------------.
# | Specific flags                                                       |
# `----------------------------------------------------------------------'

file68_shared_path='-DFILE68_SHARED_PATH=\"$(sc68datadir)\"'
if test X[$]ac_sys_native_win32 = Xyes; then
  file68_shared_path='-DFILE68_SHARED_PATH=0'
fi
SC68_ADD_FLAG(LIB_CFLAGS,[$file68_shared_path])

# ,----------------------------------------------------------------------.
# | Standard functions                                                   |
# `----------------------------------------------------------------------'

AC_CHECK_FUNCS([malloc free sleep usleep vsprintf vsnprintf getenv fsync fdatasync])

# ,----------------------------------------------------------------------.
# | Package config                                                       |
# `----------------------------------------------------------------------'

file68_features=''
if test X[$]has_curl = Xyes; then
  file68_features='curl'
  AC_DEFINE([FILE68_CURL],1,[Defined if file68 supports remote files (via libcurl)])
fi

if test X[$]has_ao = Xyes; then
  save_LIBS="[$]LIBS"
  LIBS="[$]ao_ldf [$]LIBS"
  AC_CHECK_FUNCS([ao_file_extension])
  LIBS="[$]save_LIBS"
  AC_DEFINE([FILE68_AO],1,[Defined if file68 supports audio (via libao)])
  if test -z "[$]file68_features"; then file68_features="ao";
  else file68_features="[$]file68_features,ao"; fi
fi

if test X[$]has_z = Xyes; then
  AC_DEFINE([FILE68_Z],[1],[Defined if file68 supports deflate (via zlib)])
  if test -z "[$]file68_features"; then file68_features="z";
  else file68_features="[$]file68_features,z"; fi
fi

if test X[$]has_unice68 = Xyes; then
  AC_DEFINE([FILE68_UNICE68],[1],[Defined if file68 supports !ice depacker (via unice68)])
  if test -z "[$]file68_features"; then file68_features="ice";
  else file68_features="[$]file68_features,ice"; fi
fi

AC_SUBST([file68_features])

# ,----------------------------------------------------------------------.
# | Output                                                               |
# `----------------------------------------------------------------------'

SC68_DO_ALLSTATIC

AC_CONFIG_FILES([
    Makefile
    data68/Makefile
    libfile68-${VERSION}.ebuild:libfile68.ebuild.in
    libfile68.spec
    file68.pc
    package.texi])

AC_OUTPUT

# ,----------------------------------------------------------------------.
# | Summary                                                              |
# `----------------------------------------------------------------------'

SC68_DUMPVARS

vfs=""
if test x"[$]enable_file" = xyes; then
  vfs="file"
fi

if test x"[$]enable_fd" = xyes; then
  vfs="$vfs fd"
fi

if test x"[$]enable_mem" = xyes; then
  vfs="$vfs memory"
fi
vfs="[$](echo $vfs)"

AC_MSG_NOTICE([])
AC_MSG_NOTICE([,----------------------])
AC_MSG_NOTICE([| $PACKAGE_NAME ($PACKAGE_VERSION)])
AC_MSG_NOTICE([|----------------------])
AC_MSG_NOTICE([|   compile mode        : $compile_mode])
AC_MSG_NOTICE([|   supported VFS       : $vfs])
AC_MSG_NOTICE([|   windows registry    : $enable_reg])
AC_MSG_NOTICE([|   built-in replays    : $enable_replay_rom])
AC_MSG_NOTICE([|   zlib support        : $has_z ($z_ver)])
AC_MSG_NOTICE([|   curl support        : $has_curl ($curl_ver)])
AC_MSG_NOTICE([|   curl schemes        : $enable_scheme])
AC_MSG_NOTICE([|   ICE! support        : $has_unice68 ($unice68_ver)])
AC_MSG_NOTICE([|   ao support          : $has_ao ($ao_ver)])
AC_MSG_NOTICE([|   sc68 data dir       : $sc68datadir])
AC_MSG_NOTICE([+----------------------])
