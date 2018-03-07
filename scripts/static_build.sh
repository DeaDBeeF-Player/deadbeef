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
    export GTK_ROOT_310="$ORIGIN/$STATIC_DEPS/lib-x86-32/gtk-3.10.8";
    export GTK_ROOT_216="$ORIGIN/$STATIC_DEPS/lib-x86-32/gtk-2.16.0";

    export GTK2_216_CFLAGS="-I${GTK_ROOT_216}/include/gtk-2.0 -I${GTK_ROOT_216}/lib/gtk-2.0/include -I${GTK_ROOT_216}/include/atk-1.0 -I${GTK_ROOT_216}/include/cairo -I${GTK_ROOT_216}/include/pango-1.0 -I${GTK_ROOT_216}/include -I${GTK_ROOT_216}/include/glib-2.0 -I${GTK_ROOT_216}/lib/glib-2.0/include"
    export GTK2_216_LIBS="-L${GTK_ROOT_216}/lib -lgtk-x11-2.0 -lpango-1.0 -lcairo -lgdk-x11-2.0 -lgdk_pixbuf-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"

    export GTK3_310_CFLAGS="-I${GTK_ROOT_310}/usr/include/gtk-3.0 -I${GTK_ROOT_310}/usr/include/at-spi2-atk/2.0 -I${GTK_ROOT_310}/usr/include/at-spi-2.0 -I${GTK_ROOT_310}/usr/include/dbus-1.0 -I${GTK_ROOT_310}/usr/lib/i386-linux-gnu/dbus-1.0/include -I${GTK_ROOT_310}/usr/include/gtk-3.0 -I${GTK_ROOT_310}/usr/include/gio-unix-2.0/ -I${GTK_ROOT_310}/usr/include/mirclient -I${GTK_ROOT_310}/usr/include/mircore -I${GTK_ROOT_310}/usr/include/mircookie -I${GTK_ROOT_310}/usr/include/cairo -I${GTK_ROOT_310}/usr/include/pango-1.0 -I${GTK_ROOT_310}/usr/include/harfbuzz -I${GTK_ROOT_310}/usr/include/pango-1.0 -I${GTK_ROOT_310}/usr/include/atk-1.0 -I${GTK_ROOT_310}/usr/include/cairo -I${GTK_ROOT_310}/usr/include/pixman-1 -I${GTK_ROOT_310}/usr/include/freetype2 -I${GTK_ROOT_310}/usr/include/libpng12 -I${GTK_ROOT_310}/usr/include/gdk-pixbuf-2.0 -I${GTK_ROOT_310}/usr/include/libpng12 -I${GTK_ROOT_310}/usr/include/glib-2.0 -I${GTK_ROOT_310}/usr/lib/i386-linux-gnu/glib-2.0/include"
    export GTK3_310_LIBS="-L${GTK_ROOT_310}/lib -L${GTK_ROOT_310}/usr/lib -L${GTK_ROOT_310}/usr/lib/i386-linux-gnu -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"

elif [[ "$ARCH" == "x86_64" ]]; then
    export CFLAGS="-m64 -I$ORIGIN/$STATIC_DEPS/lib-x86-64/include/x86_64-linux-gnu"
    export LDFLAGS="-m64 -L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib -L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib/x86_64-linux-gnu"
    export CONFIGURE_FLAGS="--build=x86_64-unknown-linux-gnu"
    export LD_LIBRARY_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-64/lib"
    export PKG_CONFIG_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-64/lib/pkgconfig"
    export GTK_ROOT_310="$ORIGIN/$STATIC_DEPS/lib-x86-64/gtk-3.10.8";
    export GTK_ROOT_216="$ORIGIN/$STATIC_DEPS/lib-x86-64/gtk-2.16.0";

    export GTK2_216_CFLAGS="-I${GTK_ROOT_216}/include/gtk-2.0 -I${GTK_ROOT_216}/lib/gtk-2.0/include -I${GTK_ROOT_216}/include/atk-1.0 -I${GTK_ROOT_216}/include/cairo -I${GTK_ROOT_216}/include/pango-1.0 -I${GTK_ROOT_216}/include -I${GTK_ROOT_216}/include/glib-2.0 -I${GTK_ROOT_216}/lib/glib-2.0/include"
    export GTK2_216_LIBS="-L${GTK_ROOT_216}/lib -lgtk-x11-2.0 -lpango-1.0 -lcairo -lgdk-x11-2.0 -lgdk_pixbuf-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"

    export GTK3_310_CFLAGS="-I${GTK_ROOT_310}/usr/include/gtk-3.0 -I${GTK_ROOT_310}/usr/include/at-spi2-atk/2.0 -I${GTK_ROOT_310}/usr/include/at-spi-2.0 -I${GTK_ROOT_310}/usr/include/dbus-1.0 -I${GTK_ROOT_310}/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I${GTK_ROOT_310}/usr/include/gtk-3.0 -I${GTK_ROOT_310}/usr/include/gio-unix-2.0/ -I${GTK_ROOT_310}/usr/include/mirclient -I${GTK_ROOT_310}/usr/include/mircore -I${GTK_ROOT_310}/usr/include/mircookie -I${GTK_ROOT_310}/usr/include/cairo -I${GTK_ROOT_310}/usr/include/pango-1.0 -I${GTK_ROOT_310}/usr/include/harfbuzz -I${GTK_ROOT_310}/usr/include/pango-1.0 -I${GTK_ROOT_310}/usr/include/atk-1.0 -I${GTK_ROOT_310}/usr/include/cairo -I${GTK_ROOT_310}/usr/include/pixman-1 -I${GTK_ROOT_310}/usr/include/freetype2 -I${GTK_ROOT_310}/usr/include/libpng12 -I${GTK_ROOT_310}/usr/include/gdk-pixbuf-2.0 -I${GTK_ROOT_310}/usr/include/libpng12 -I${GTK_ROOT_310}/usr/include/glib-2.0 -I${GTK_ROOT_310}/usr/lib/x86_64-linux-gnu/glib-2.0/include"
    export GTK3_310_LIBS="-L${GTK_ROOT_310}/lib -L${GTK_ROOT_310}/usr/lib -L${GTK_ROOT_310}/usr/lib/x86_64-linux-gnu -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"
else
    echo unknown arch $ARCH
    exit 1
fi

cd tools/apbuild
./apinit || exit 1
cd ../../

export APBUILD_STATIC_LIBGCC=1
export APBUILD_CXX1=1
export CC=$AP/apgcc
export CXX=$AP/apgcc
export OBJC=$AP/apgcc

./autogen.sh || exit 1

./configure CFLAGS="$CFLAGS -O3 -D_FORTIFY_SOURCE=0" CXXFLAGS="$CXXFLAGS -O3 -D_FORTIFY_SOURCE=0" LDFLAGS="$LDFLAGS" $CONFIGURE_FLAGS --enable-staticlink --disable-artwork-imlib2 --prefix=/opt/deadbeef || exit 1
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
sed -i 's/hardcode_into_libs=yes/hardcode_into_libs=no/g' libtool
make clean
make -j8 DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION || exit 1
make DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION install || exit 1

MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
    echo "building pluginfo tool..."
    cd tools/pluginfo
    make || exit 1
    cd ../../
fi
