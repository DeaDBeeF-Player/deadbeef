#!/bin/sh

# package for distribution
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
BUILD=`cat PORTABLE_BUILD | perl -ne 'chomp and print'`

# main distro
SRCDIR=deadbeef-$VERSION-portable
PLUGDIR=$SRCDIR/plugins
DOCDIR=$SRCDIR/doc
PIXMAPDIR=$SRCDIR/pixmaps

tar jcvf deadbeef-$VERSION-portable-build$BUILD.tar.bz2\
    $SRCDIR/deadbeef\
    $SRCDIR/deadbeef.png\
    $DOCDIR\
    $PLUGDIR/alsa.so\
    $PLUGDIR/oss.so\
    $PLUGDIR/vfs_curl.so\
    $PLUGDIR/artwork.so\
    $PLUGDIR/gtkui.so\
    $PLUGDIR/hotkeys.so\
    $PLUGDIR/cdda.so\
    $PLUGDIR/mpgmad.so\
    $PLUGDIR/vorbis.so\
    $PLUGDIR/wavpack.so\
    $PLUGDIR/flac.so\
    $PLUGDIR/ffap.so\
    $PLUGDIR/musepack.so\
    $PLUGDIR/notify.so\
    $PLUGDIR/sndfile.so\
    $PLUGDIR/supereq.so\
    $PLUGDIR/tta.so\
    $PIXMAPDIR

