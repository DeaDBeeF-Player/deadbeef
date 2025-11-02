#!/bin/bash
VERSION=$(<"build_data/VERSION")
ORIGIN=$(pwd)
AP=$ORIGIN/tools/apbuild
export CC=$AP/apgcc
export CXX=$AP/apgcc 

export APBUILD_STATIC_LIBGCC=1

./autogen.sh

./configure --enable-staticlink --enable-portable --disable-artwork-imlib2
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
make clean
make -j9

#./scripts/portable_extraplugs.sh

cd $ORIGIN

echo "building pluginfo tool..."
cd tools/pluginfo
make
cd ../../

