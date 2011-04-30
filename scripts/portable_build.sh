#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
export APBUILD_STATIC_LIBGCC=1

./autogen.sh
export CC=$ORIGIN/tools/apbuild/apgcc
export CXX=$ORIGIN/tools/apbuild/apgcc

./configure --enable-staticlink --enable-portable --disable-nls --disable-artwork-imlib2
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
make clean
make -j9

#ZLIB_LIBS="$ORIGIN/lib-x86-32/libz.a"
CFLAGS="-I$ORIGIN/lib-x86-32/include"

for i in shn dumb ao ; do
    echo cd
    cd $ORIGIN/plugins/$i
    make clean
    echo making $ORIGIN/plugins/$i
    make -j8 STATIC_CFLAGS="$CFLAGS" CC=$CC CXX=$CXX
done

cd $ORIGIN

echo "building pluginfo tool..."
cd tools/pluginfo
make
cd ../../

./scripts/portable_postbuild.sh

