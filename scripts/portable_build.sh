#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
export APBUILD_STATIC_LIBGCC=1

CC=$ORIGIN/tools/apbuild/apgcc CXX=$ORIGIN/tools/apbuild/apgcc ./configure --enable-staticlink --enable-portable=yes --disable-pulse --disable-mpris --disable-nls
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
make clean
make -j9

echo "building pluginfo tool..."
cd tools/pluginfo
make
cd ../../

./scripts/portable_postbuild.sh

