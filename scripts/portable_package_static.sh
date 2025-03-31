#!/bin/bash

./scripts/portable_postbuild.sh

# package for distribution
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
BUILD=`cat PORTABLE_BUILD | perl -ne 'chomp and print'`
if [[ "$ARCH" == "i686" ]]; then
    echo arch: $ARCH
elif [[ "$ARCH" == "x86_64" ]]; then
    echo arch: $ARCH
else
    echo unknown arch $ARCH
    exit 1
fi

# main distro
SRCDIR=deadbeef-$VERSION
PLUGDIR=$SRCDIR/plugins
LIBDIR=$SRCDIR/lib
DOCDIR=$SRCDIR/doc
PIXMAPDIR=$SRCDIR/pixmaps
OUTNAME=deadbeef-static_${VERSION}-${BUILD}_${ARCH}.tar.bz2

mkdir -p portable_out/build
rm portable_out/build/$OUTNAME

cd portable/$ARCH
tar jcvf ../../portable_out/build/$OUTNAME\
    $SRCDIR/deadbeef\
    $SRCDIR/deadbeef.png\
    $DOCDIR\
    $LIBDIR\
    $PLUGDIR/aac.so\
    $PLUGDIR/adplug.so\
    $PLUGDIR/alsa.so\
    $PLUGDIR/artwork.so\
    $PLUGDIR/cdda.so\
    $PLUGDIR/dca.so\
    $PLUGDIR/ddb_gui_GTK2.so\
    $PLUGDIR/ddb_gui_GTK3.so\
    $PLUGDIR/ffap.so\
    $PLUGDIR/ffmpeg.so\
    $PLUGDIR/flac.so\
    $PLUGDIR/gme.so\
    $PLUGDIR/hotkeys.so\
    $PLUGDIR/lastfm.so\
    $PLUGDIR/m3u.so\
    $PLUGDIR/mms.so\
    $PLUGDIR/mp3.so\
    $PLUGDIR/musepack.so\
    $PLUGDIR/notify.so\
    $PLUGDIR/nullout.so\
    $PLUGDIR/oss.so\
    $PLUGDIR/shellexec.so\
    $PLUGDIR/shellexecui_gtk2.so\
    $PLUGDIR/shellexecui_gtk3.so\
    $PLUGDIR/sid.so\
    $PLUGDIR/sndfile.so\
    $PLUGDIR/supereq.so\
    $PLUGDIR/tta.so\
    $PLUGDIR/vfs_curl.so\
    $PLUGDIR/vfs_zip.so\
    $PLUGDIR/vorbis.so\
    $PLUGDIR/opus.so\
    $PLUGDIR/vtx.so\
    $PLUGDIR/wavpack.so\
    $PLUGDIR/wildmidi.so\
    $PLUGDIR/psf.so\
    $PLUGDIR/ddb_shn.so\
    $PLUGDIR/ddb_dumb.so\
    $PLUGDIR/converter.so\
    $PLUGDIR/converter_gtk2.so\
    $PLUGDIR/converter_gtk3.so\
    $PLUGDIR/convpresets\
    $PLUGDIR/pulse.so\
    $PLUGDIR/ddb_out_pw.so\
    $PLUGDIR/ddb_dsp_libretro.so\
    $PLUGDIR/dsp_libsrc.so\
    $PLUGDIR/ddb_mono2stereo.so\
    $PLUGDIR/alac.so\
    $PLUGDIR/wma.so\
    $PLUGDIR/rg_scanner.so\
    $PLUGDIR/pltbrowser_gtk2.so\
    $PLUGDIR/pltbrowser_gtk3.so\
    $PLUGDIR/in_sc68.so\
    $PLUGDIR/ddb_soundtouch.so\
    $PLUGDIR/data68\
    $PLUGDIR/medialib.so\
    $PLUGDIR/lyrics_gtk2.so\
    $PLUGDIR/lyrics_gtk3.so\
    $PIXMAPDIR\
    $SRCDIR/locale\
    || exit 1

cd ../..
