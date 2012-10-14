#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
AP=$ORIGIN/tools/apbuild
ARCH=`uname -m | perl -ne 'chomp and print'`

cd tools/apbuild
./apinit
cd ../../

./autogen.sh

export APBUILD_STATIC_LIBGCC=1
CC=$AP/apgcc CXX=$AP/apgcc ./configure --enable-staticlink --disable-artwork-imlib2 --enable-ffmpeg --prefix=/opt/deadbeef
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
make clean
make DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION -j8 install

echo "building pluginfo tool..."
cd tools/pluginfo
make
cd ../../
