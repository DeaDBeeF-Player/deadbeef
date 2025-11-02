#!/bin/bash
set -e
set -x
VERSION=$(<"build_data/VERSION")
OSTYPE=`uname -s`
if [[ "$ARCH" == "i686" ]]; then
    echo arch: $ARCH
elif [[ "$ARCH" == "x86_64" ]]; then
    echo arch: $ARCH
else
    echo unknown arch $ARCH
    exit -1
fi

DEBUG=false

for arg in "$@"; do
    if [[ "$arg" == "--debug" ]]; then
        DEBUG=true
        break
    fi
done

OUTDIR=portable/$ARCH/deadbeef-$VERSION
PLUGDIR=$OUTDIR/plugins
DOCDIR=$OUTDIR/doc
PIXMAPDIR=$OUTDIR/pixmaps
echo OUTDIR=$OUTDIR

rm -rf $OUTDIR

mkdir -p $PLUGDIR
mkdir -p $DOCDIR
mkdir -p $PIXMAPDIR

cp src/deadbeef $OUTDIR

for i in converter pltbrowser shellexecui lyrics ; do
    if [ -f ./plugins/$i/.libs/${i}_gtk2.so ]; then
        cp ./plugins/$i/.libs/${i}_gtk2.so $PLUGDIR/
    else
        echo ./plugins/$i/.libs/$i_gtk2.so not found
    fi
    if [ -f ./plugins/$i/.libs/${i}_gtk3.so ]; then
        cp ./plugins/$i/.libs/${i}_gtk3.so $PLUGDIR/
    else
        echo ./plugins/$i/.libs/$i_gtk3.so not found
    fi

    if [ -f ./plugins/$i/.libs/$i.fallback.so ]; then
        cp ./plugins/$i/.libs/$i.fallback.so $PLUGDIR/
    fi
done

for i in nullout cdda flac alsa mp3 hotkeys vtx \
     ffap ffmpeg wavpack vorbis opus oss vfs_curl \
     lastfm sid adplug sndfile alac \
     supereq gme dumb notify musepack wildmidi \
     tta dca aac mms shn psf shellexec vfs_zip \
     m3u converter pulse dsp_libsrc mono2stereo \
     wma rg_scanner soundtouch artwork medialib\
     ; do
    if [ -f ./plugins/$i/.libs/$i.so ]; then
        cp ./plugins/$i/.libs/$i.so $PLUGDIR/
    elif [ -f ./plugins/$i/$i.so ]; then
        cp ./plugins/$i/$i.so $PLUGDIR/
    elif [ -f ./plugins/$i/.libs/ddb_$i.so ]; then
        cp ./plugins/$i/.libs/ddb_$i.so $PLUGDIR/
    else
        echo ./plugins/$i/.libs/$i.so not found
    fi
done

if [ -f ./plugins/gtkui/.libs/ddb_gui_GTK2.so ]; then
    cp ./plugins/gtkui/.libs/ddb_gui_GTK2.so $PLUGDIR/
else
    echo ./plugins/gtkui/.libs/ddb_gui_GTK2.so not found
fi

if [ -f ./plugins/gtkui/.libs/ddb_gui_GTK3.so ]; then
    cp ./plugins/gtkui/.libs/ddb_gui_GTK3.so $PLUGDIR/
else
    echo ./plugins/gtkui/.libs/ddb_gui_GTK3.so not found
fi

cp ./external/ddb_output_pw/.libs/ddb_out_pw.so $PLUGDIR/
cp ./external/ddb_dsp_libretro/.libs/ddb_dsp_libretro.so $PLUGDIR/

#pixmaps

for i in pause_16.png play_16.png noartwork.png buffering_16.png; do
    cp ./pixmaps/$i $PIXMAPDIR/
done

# docs
for i in ChangeLog help.txt COPYING.GPLv2 COPYING.LGPLv2.1 about.txt translators.txt; do
    cp ./$i $DOCDIR/
done

# icon
cp ./icons/32x32/deadbeef.png $OUTDIR/

# converter presets
cp -r plugins/converter/convpresets $OUTDIR/plugins/

# sc68data
cp -r plugins/sc68/.libs/in_sc68.so $OUTDIR/plugins/
mkdir -p  $OUTDIR/plugins/data68/Replay
cp -r plugins/sc68/libsc68/file68/data68/Replay/*.bin $OUTDIR/plugins/data68/Replay/

# dynamic libs
mkdir -p $OUTDIR/lib
cp -r static-deps/lib-x86-64/lib/libBlocksRuntime.so* $OUTDIR/lib/
cp -r static-deps/lib-x86-64/lib/libdispatch.so* $OUTDIR/lib/
cp -r static-deps/lib-x86-64/lib/libcurl.so* $OUTDIR/lib/
cp -r static-deps/lib-x86-64/lib/libmbed*.so* $OUTDIR/lib/

# translations
mkdir -p $OUTDIR/locale
for i in po/*.gmo ; do
    base=`basename po/$i .gmo`
    mkdir -p $OUTDIR/locale/$base/LC_MESSAGES
    cp $i $OUTDIR/locale/$base/LC_MESSAGES/deadbeef.mo
done

if ! $DEBUG; then
# strip
    if [ $OSTYPE != 'Darwin' ];then
        strip --strip-unneeded $OUTDIR/deadbeef
        for i in $PLUGDIR/*.so ; do strip --strip-unneeded $i ; done
    fi
fi
