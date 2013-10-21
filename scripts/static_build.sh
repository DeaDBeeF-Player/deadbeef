#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
AP=$ORIGIN/tools/apbuild
ARCH=`uname -m | perl -ne 'chomp and print'`

cd tools/apbuild
./apinit
cd ../../

export APBUILD_STATIC_LIBGCC=1
export CC=$AP/apgcc
export CXX=$AP/apgcc

./autogen.sh

./configure --enable-staticlink --disable-artwork-imlib2 --prefix=/opt/deadbeef
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
sed -i 's/hardcode_into_libs=yes/hardcode_into_libs=no/g' libtool
make clean
make DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION 
make DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION install

echo "building pluginfo tool..."
cd tools/pluginfo
make
cd ../../
