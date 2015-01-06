#!/bin/bash
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=`pwd | perl -ne 'chomp and print'`
STATIC_DEPS=static-deps
AP=$ORIGIN/tools/apbuild
#ARCH=`uname -m | perl -ne 'chomp and print'`
if [[ "$ARCH" == "i686" ]]; then
    export CFLAGS="-m32 -I$ORIGIN/$STATIC_DEPS/lib-x86-32/include/i386-linux-gnu"
    export CXXFLAGS=$CFLAGS
    export LDFLAGS="-m32 -L$ORIGIN/$STATIC_DEPS/lib-x86-32/lib -L$ORIGIN/$STATIC_DEPS/lib-x86-32/lib/i386-linux-gnu"
    export CONFIGURE_FLAGS="--build=i686-unknown-linux-gnu"
    export LD_LIBRARY_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-32/lib"
    export PKG_CONFIG_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-32/lib/pkgconfig"
    export GTK_ROOT_300="$ORIGIN/$STATIC_DEPS/lib-x86-32/gtk-3.0.0";
    export GTK_ROOT_216="$ORIGIN/$STATIC_DEPS/lib-x86-32/gtk-2.16.0";
    export GTK_ROOT_212="$ORIGIN/$STATIC_DEPS/lib-x86-32/gtk-2.12.12";
elif [[ "$ARCH" == "x86_64" ]]; then
    export CFLAGS="-m64 -I$ORIGIN/$STATIC_DEPS/lib-x86-64/include/x86_64-linux-gnu"
    export LDFLAGS="-m64 -L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib -L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib/x86_64-linux-gnu"
    export CONFIGURE_FLAGS="--build=x86_64-unknown-linux-gnu"
    export LD_LIBRARY_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-64/lib"
    export PKG_CONFIG_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-64/lib/pkgconfig"
    export GTK_ROOT_300="$ORIGIN/$STATIC_DEPS/lib-x86-64/gtk-3.0.0";
    export GTK_ROOT_216="$ORIGIN/$STATIC_DEPS/lib-x86-64/gtk-2.16.0";
    export GTK_ROOT_212="$ORIGIN/$STATIC_DEPS/lib-x86-64/gtk-2.12.12";
else
    echo unknown arch $ARCH
    exit -1
fi

cd tools/apbuild
./apinit || exit -1
cd ../../

export APBUILD_STATIC_LIBGCC=1
export APBUILD_CXX1=1
export CC=$AP/apgcc
export CXX=$AP/apgcc
export OBJC=$AP/apgcc

./autogen.sh || exit -1

./configure CFLAGS="$CFLAGS -O3 -D_FORTIFY_SOURCE=0" CXXFLAGS="$CXXFLAGS -O3 -D_FORTIFY_SOURCE=0" LDFLAGS="$LDFLAGS" $CONFIGURE_FLAGS --enable-staticlink --disable-artwork-imlib2 --prefix=/opt/deadbeef || exit -1
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
sed -i 's/hardcode_into_libs=yes/hardcode_into_libs=no/g' libtool
make clean
make V=0 -j8 DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION || exit -1
make V=0 DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION install || exit -1

echo "building pluginfo tool..."
cd tools/pluginfo
make || exit -1
cd ../../
