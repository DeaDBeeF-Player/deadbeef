#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
AP=$ORIGIN/tools/apbuild
ARCH=`uname -m | perl -ne 'chomp and print'`

./autogen.sh

export APBUILD_STATIC_LIBGCC=1
CC=$AP/apgcc CXX=$AP/apgcc  ./configure --enable-staticlink=yes --disable-artwork-imlib2 --prefix=/opt/deadbeef
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
make DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION -j8 install

