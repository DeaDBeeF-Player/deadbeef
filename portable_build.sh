#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
export APBUILD_STATIC_LIBGCC=1

#CC=$ORIGIN/tools/apbuild/apgcc CXX=$ORIGIN/tools/apbuild/apgcc ./configure --enable-portable --disable-pulse --disable-mpris --enable-maintainer-mode --disable-nls
#sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
#make clean

echo "building low-end gtkui version"
cd plugins/gtkui
make clean
make CFLAGS="-DULTRA_COMPATIBLE=1"
cp .libs/gtkui.so gtkui.fallback.so
make clean
cd ../..
make -j9

echo "building pluginfo tool..."
cd tools/pluginfo
make
cd ../../


./portable_postbuild.sh

