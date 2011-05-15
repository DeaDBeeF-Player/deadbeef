#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
export APBUILD_STATIC_LIBGCC=1

./autogen.sh
export CC=$ORIGIN/tools/apbuild/apgcc
export CXX=$ORIGIN/tools/apbuild/apgcc

./configure --enable-staticlink --enable-portable --disable-artwork-imlib2
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
make clean
make -j9

./scripts/portable_extraplugs.sh

cd $ORIGIN

echo "building pluginfo tool..."
cd tools/pluginfo
make
cd ../../

./scripts/portable_postbuild.sh

