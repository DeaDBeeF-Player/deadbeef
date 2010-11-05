#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
OUTDIR=deadbeef-$VERSION-portable
PLUGDIR=$OUTDIR/plugins
DOCDIR=$OUTDIR/doc
PIXMAPDIR=$OUTDIR/pixmaps
mkdir -p $PLUGDIR
mkdir -p $DOCDIR
mkdir -p $PIXMAPDIR

cp ./deadbeef ./deadbeef-$VERSION-portable/
cp ./plugins/nullout/.libs/nullout.so $PLUGDIR/
cp ./plugins/cdda/.libs/cdda.so $PLUGDIR/
cp ./plugins/flac/.libs/flac.so $PLUGDIR/
cp ./plugins/alsa/.libs/alsa.so $PLUGDIR/
cp ./plugins/mpgmad/.libs/mpgmad.so $PLUGDIR/
cp ./plugins/hotkeys/.libs/hotkeys.so $PLUGDIR/
cp ./plugins/vtx/.libs/vtx.so $PLUGDIR/
cp ./plugins/ffap/.libs/ffap.so $PLUGDIR/
cp ./plugins/ffmpeg/.libs/ffmpeg.so $PLUGDIR/
cp ./plugins/wavpack/.libs/wavpack.so $PLUGDIR/
cp ./plugins/vorbis/.libs/vorbis.so $PLUGDIR/
cp ./plugins/oss/.libs/oss.so $PLUGDIR/
cp ./plugins/vfs_curl/.libs/vfs_curl.so $PLUGDIR/
cp ./plugins/lastfm/.libs/lastfm.so $PLUGDIR/
cp ./plugins/sid/.libs/sid.so $PLUGDIR/
cp ./plugins/adplug/.libs/adplug.so $PLUGDIR/
cp ./plugins/gtkui/.libs/gtkui.so $PLUGDIR/
cp ./plugins/gtkui/gtkui.fallback.so $PLUGDIR/
cp ./plugins/sndfile/.libs/sndfile.so $PLUGDIR/
cp ./plugins/artwork/.libs/artwork.so $PLUGDIR/
cp ./plugins/supereq/.libs/supereq.so $PLUGDIR/
cp ./plugins/gme/.libs/gme.so $PLUGDIR/
cp ./plugins/dumb/.libs/dumb.so $PLUGDIR/
cp ./plugins/notify/.libs/notify.so $PLUGDIR/
cp ./plugins/musepack/.libs/musepack.so $PLUGDIR/
cp ./plugins/wildmidi/.libs/wildmidi.so $PLUGDIR/
cp ./plugins/tta/.libs/tta.so $PLUGDIR/
cp ./plugins/dca/.libs/dca.so $PLUGDIR/
cp ./plugins/aac/.libs/aac.so $PLUGDIR/
cp ./plugins/mms/.libs/mms.so $PLUGDIR/
cp ./plugins/shn/.libs/shn.so $PLUGDIR/
cp ./plugins/ao/.libs/ao.so $PLUGDIR/
cp ./plugins/shellexec/.libs/shellexec.so $PLUGDIR/

#pixmaps
cp ./pixmaps/pause_16.png $PIXMAPDIR/
cp ./pixmaps/play_16.png $PIXMAPDIR/
cp ./pixmaps/buffering_16.png $PIXMAPDIR/
cp ./pixmaps/noartwork.jpg $PIXMAPDIR/

# docs
cp ./ChangeLog $DOCDIR/
cp ./help.txt $DOCDIR/
cp ./COPYING.GPLv2 $DOCDIR/
cp ./about.txt $DOCDIR/
cp ./translators.txt $DOCDIR/

# icon
cp ./icons/32x32/deadbeef.png $OUTDIR/

# strip
strip --strip-unneeded ./deadbeef-$VERSION-portable/deadbeef
for i in $PLUGDIR/*.so ; do strip --strip-unneeded $i ; done
