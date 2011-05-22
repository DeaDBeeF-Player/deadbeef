#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
AP=$HOME/bin/autopackage
export CC=$AP/apbuild/apgcc
export CXX=$AP/apbuild/apgcc 
export APBUILD_STATIC_LIBGCC=1

ZLIB_LIBS="$ORIGIN/../deadbeef-deps/lib-x86-32/lib/libz.a"
CFLAGS="-I$ORIGIN/../deadbeef-deps/lib-x86-32/include"

for i in shn dumb ao ; do
    echo cd
    cd $ORIGIN/plugins/$i
    make clean
    echo making $ORIGIN/plugins/$i
    make -j8 STATIC_CFLAGS="$CFLAGS" CC=$CC CXX=$CXX ZLIB_LIBS=$ZLIB_LIBS
done

