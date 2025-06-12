#!/bin/bash
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ORIGIN=$PWD
STATIC_DEPS=static-deps
AP=$ORIGIN/external/apbuild
#ARCH=`uname -m | perl -ne 'chomp and print'`
if [[ "$ARCH" == "i686" ]]; then
    export CFLAGS="-m32 -I$ORIGIN/$STATIC_DEPS/lib-x86-32/include"
    export CXXFLAGS=$CFLAGS
    export LDFLAGS="-m32 -L$ORIGIN/$STATIC_DEPS/lib-x86-32/lib -L$ORIGIN/$STATIC_DEPS/lib-x86-32/lib/i386-linux-gnu"
    export CONFIGURE_FLAGS="--build=i686-unknown-linux-gnu"
    export LIBRARY_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-32/lib"
    export PKG_CONFIG_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-32/lib/pkgconfig"
    export GTK_ROOT_310="$ORIGIN/$STATIC_DEPS/lib-x86-32/gtk-3.10.8";
    export GTK_ROOT_216="$ORIGIN/$STATIC_DEPS/lib-x86-32/gtk-2.16.0";

    export GTK2_216_CFLAGS="-I${GTK_ROOT_216}/include/gtk-2.0 -I${GTK_ROOT_216}/lib/gtk-2.0/include -I${GTK_ROOT_216}/include/atk-1.0 -I${GTK_ROOT_216}/include/cairo -I${GTK_ROOT_216}/include/pango-1.0 -I${GTK_ROOT_216}/include -I${GTK_ROOT_216}/include/glib-2.0 -I${GTK_ROOT_216}/lib/glib-2.0/include"
    export GTK2_216_LIBS="-L${GTK_ROOT_216}/lib -lgtk-x11-2.0 -lpango-1.0 -lcairo -lgdk-x11-2.0 -lgdk_pixbuf-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"

    export GTK3_310_CFLAGS="-I${GTK_ROOT_310}/include/gtk-3.0 -I${GTK_ROOT_310}/include/at-spi2-atk/2.0 -I${GTK_ROOT_310}/include/at-spi-2.0 -I${GTK_ROOT_310}/include/dbus-1.0 -I${GTK_ROOT_310}/lib/i386-linux-gnu/dbus-1.0/include -I${GTK_ROOT_310}/include/gtk-3.0 -I${GTK_ROOT_310}/include/gio-unix-2.0/ -I${GTK_ROOT_310}/include/mirclient -I${GTK_ROOT_310}/include/mircore -I${GTK_ROOT_310}/include/mircookie -I${GTK_ROOT_310}/include/cairo -I${GTK_ROOT_310}/include/pango-1.0 -I${GTK_ROOT_310}/include/harfbuzz -I${GTK_ROOT_310}/include/pango-1.0 -I${GTK_ROOT_310}/include/atk-1.0 -I${GTK_ROOT_310}/include/cairo -I${GTK_ROOT_310}/include/pixman-1 -I${GTK_ROOT_310}/include/freetype2 -I${GTK_ROOT_310}/include/libpng12 -I${GTK_ROOT_310}/include/gdk-pixbuf-2.0 -I${GTK_ROOT_310}/include/libpng12 -I${GTK_ROOT_310}/include/glib-2.0 -I${GTK_ROOT_310}/lib/glib-2.0/include"
    export GTK3_310_LIBS="-L${GTK_ROOT_310}/lib -L${GTK_ROOT_310}/lib -L${GTK_ROOT_310}/lib/i386-linux-gnu -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"

elif [[ "$ARCH" == "x86_64" ]]; then
    export CFLAGS="-m64 -I$ORIGIN/$STATIC_DEPS/lib-x86-64/include"
    export LDFLAGS="-m64 -L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib -L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib/x86_64-linux-gnu"
    export CONFIGURE_FLAGS="--build=x86_64-unknown-linux-gnu"
# Keeping this here for debugging (faster builds)
#    export CONFIGURE_FLAGS="--build=x86_64-unknown-linux-gnu --disable-gtk2 --disable-gtk3 --disable-gme --disable-sid --disable-dumb --disable-psf --disable-nls --disable-adplug --disable-converter --disable-aac --disable-pltbrowser --disable-soundtouch --disable-mp3 --disable-oss --disable-alsa --disable-oss --disable-pipewire --disable-vtx --disable-sc68 --disable-musepack --disable-tta --disable-dca --disable-mms --disable-m3u --disable-shn --disable-mono2stereo --disable-wildmidi --disable-shellexec --disable-notify --disable-pulse --disable-supereq --disable-ffap"
    export LIBRARY_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-64/lib"
    export PKG_CONFIG_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-64/lib/pkgconfig"
    export GTK_ROOT_310="$ORIGIN/$STATIC_DEPS/lib-x86-64/gtk-3.10.8";
    export GTK_ROOT_216="$ORIGIN/$STATIC_DEPS/lib-x86-64/gtk-2.16.0";

    export GTK2_216_CFLAGS="-I${GTK_ROOT_216}/include/gtk-2.0 -I${GTK_ROOT_216}/lib/gtk-2.0/include -I${GTK_ROOT_216}/include/atk-1.0 -I${GTK_ROOT_216}/include/cairo -I${GTK_ROOT_216}/include/pango-1.0 -I${GTK_ROOT_216}/include -I${GTK_ROOT_216}/include/glib-2.0 -I${GTK_ROOT_216}/lib/glib-2.0/include"
    export GTK2_216_LIBS="-L${GTK_ROOT_216}/lib -lgtk-x11-2.0 -lpango-1.0 -lcairo -lgdk-x11-2.0 -lgdk_pixbuf-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"

    export GDK_PIXBUF_LIBS="-L${GTK_ROOT_216}/lib -lgdk_pixbuf-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"

    export GTK3_310_CFLAGS="-I${GTK_ROOT_310}/include/gtk-3.0 -I${GTK_ROOT_310}/include/at-spi2-atk/2.0 -I${GTK_ROOT_310}/include/at-spi-2.0 -I${GTK_ROOT_310}/include/dbus-1.0 -I${GTK_ROOT_310}/lib/dbus-1.0/include -I${GTK_ROOT_310}/include/gtk-3.0 -I${GTK_ROOT_310}/include/gio-unix-2.0/ -I${GTK_ROOT_310}/include/mirclient -I${GTK_ROOT_310}/include/mircore -I${GTK_ROOT_310}/include/mircookie -I${GTK_ROOT_310}/include/cairo -I${GTK_ROOT_310}/include/pango-1.0 -I${GTK_ROOT_310}/include/harfbuzz -I${GTK_ROOT_310}/include/pango-1.0 -I${GTK_ROOT_310}/include/atk-1.0 -I${GTK_ROOT_310}/include/cairo -I${GTK_ROOT_310}/include/pixman-1 -I${GTK_ROOT_310}/include/freetype2 -I${GTK_ROOT_310}/include/libpng12 -I${GTK_ROOT_310}/include/gdk-pixbuf-2.0 -I${GTK_ROOT_310}/include/libpng12 -I${GTK_ROOT_310}/include/glib-2.0 -I${GTK_ROOT_310}/lib/glib-2.0/include"
    export GTK3_310_LIBS="-L${GTK_ROOT_310}/lib -L${GTK_ROOT_310}/lib -L${GTK_ROOT_310}/lib/x86_64-linux-gnu -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lgthread-2.0 -lglib-2.0"
else
    echo unknown arch $ARCH
    exit 1
fi

export LD_LIBRARY_PATH=$LIBRARY_PATH

# using clang requires higher version of libstdc++, than provided in staticdeps, so remove it
rm static-deps/lib-x86-64/lib/libstdc*

# static-deps build contains some dynamic libs which we don't want.
# Delete them, so that libtool will always link static libs.
find static-deps/lib-x86-64/lib/ -type f -name "*libcddb*so*" -o -name "*sndfile*so*" -o -name "*faad*so*" -o -name "*opencore*so*" -o -name "*libz*so*" -o -name "*libzip*so*" -o -name "*libav*so*" -o -name "*libopus*so*" -o -name "*dbus*so*" -o -name "*libexpat*so*" -o -name "*libmad*so*" -o -name "*libmpg123*so*" -o -name "*wavpack*so*" -o -name "*samplerate*so*" | xargs rm

# setup apgcc environment
cd external/apbuild
./apinit || exit 1
cd ../../
export APBUILD_STATIC_LIBGCC=1
export APBUILD_CXX1=1
export APGCC_USE_CLANG=1
export CC=$AP/apgcc
export CXX=$AP/apgcc
export OBJC=$AP/apgcc

./autogen.sh || exit 1

CFLAGS="$CFLAGS -g"
LDFLAGS="$LDFLAGS -Wl,--build-id"

./configure CFLAGS="$CFLAGS -O3 -D_FORTIFY_SOURCE=0" CXXFLAGS="$CXXFLAGS -O3 -D_FORTIFY_SOURCE=0" LDFLAGS="$LDFLAGS" $CONFIGURE_FLAGS --enable-staticlink --prefix=/opt/deadbeef || {
    # store failed config.log in portable dir, which is mapped to
    # docker-artifacts when using docker.
    cp config.log ./portable/
    cp config.h ./portable/
    cp config.h.in ./portable/
    exit 1
}
sed -i 's/-lstdc++ -lm -lgcc_s -lc -lgcc_s/-lm -lc/g' libtool
sed -i 's/hardcode_into_libs=yes/hardcode_into_libs=no/g' libtool
make clean
make V=1 -j8 DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION || exit 1
export DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION

echo "Extracting debug symbols..."
# Note: need to do it before make install!
mkdir -p "portable/$ARCH"
cd src
objcopy --only-keep-debug ./deadbeef deadbeef.debug || exit 1
strip --strip-debug --strip-unneeded ./deadbeef || exit 1
objcopy --add-gnu-debuglink=deadbeef.debug ./deadbeef || exit 1
cd ..
mv src/deadbeef.debug "portable/$ARCH/" || exit 1

make DESTDIR=$DESTDIR install || exit 1
mkdir -p $LIBRARY_PATH
cp -r $LIBRARY_PATH/libBlocksRuntime.so* $DESTDIR/opt/deadbeef/lib/
cp -r $LIBRARY_PATH/libdispatch.so* $DESTDIR/opt/deadbeef/lib/
cp -r $LIBRARY_PATH/libcurl.so* $DESTDIR/opt/deadbeef/lib/
cp -r $LIBRARY_PATH/libmbed*.so* $DESTDIR/opt/deadbeef/lib/

# Check that all built plugins are linked against glibc version 2.17 max
find $DESTDIR/opt/deadbeef -type f -name "*.so" | while read i ; do ./scripts/glibc-check.sh "$i" "2.17"; done

MACHINE_TYPE=`uname -m`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
    echo "Building pluginfo tool..."
    cd tools/pluginfo
    make || exit 1
    cd ../../
fi
