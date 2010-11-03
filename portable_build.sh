#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`

CC=tools/apbuild/apgcc CXX=tools/apbuild/apgcc ./configure --enable-portable --disable-pulse --enable-maintainer-mode --disable-nls

sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
#make clean
make -j9

#./portable_postbuild.sh

cd tools/pluginfo
make
cd ../../
