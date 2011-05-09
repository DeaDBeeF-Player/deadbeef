#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
export APBUILD_STATIC_LIBGCC=1
export CC=$ORIGIN/tools/apbuild/apgcc
export CXX=$ORIGIN/tools/apbuild/apgcc

ZLIB_LIBS="$ORIGIN/lib-x86-32/libz.a"
CFLAGS="-I$ORIGIN/lib-x86-32/include"

for i in shn dumb ao ; do
    echo cd
    cd $ORIGIN/plugins/$i
    make clean
    echo making $ORIGIN/plugins/$i
    make -j8 STATIC_CFLAGS="$CFLAGS" CC=$CC CXX=$CXX ZLIB_LIBS=$ZLIB_LIBS
done

