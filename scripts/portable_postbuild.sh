#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
OSTYPE=`uname -s`
OUTDIR=deadbeef-$VERSION-portable
PLUGDIR=$OUTDIR/plugins
DOCDIR=$OUTDIR/doc
PIXMAPDIR=$OUTDIR/pixmaps
mkdir -p $PLUGDIR
mkdir -p $DOCDIR
mkdir -p $PIXMAPDIR

cp ./deadbeef ./deadbeef-$VERSION-portable/

for i in nullout cdda flac alsa mpgmad hotkeys vtx \
	 ffap ffmpeg wavpack vorbis oss vfs_curl \
	 lastfm sid adplug sndfile artwork \
	 supereq gme dumb notify musepack wildmidi \
	 tta dca aac mms shn ao shellexec; do
	 if [ -f ./plugins/$i/.libs/$i.so ]; then
		 cp ./plugins/$i/.libs/$i.so $PLUGDIR/
	else
		echo $i not found
	fi
done

for i in gtkui gtkui.fallback;do
	if [ -f ./plugins/$i/.libs/$i.so ]; then
		cp ./plugins/$i/.libs/$i.so $PLUGDIR/
	else
		echo $i not found
	fi
done

#pixmaps

for i in pause_16.png play_16.png noartwork.jpg buffering_16.png; do
	cp ./pixmaps/$i $PIXMAPDIR/
done

# docs
for i in ChangeLog help.txt COPYING.GPLv2 about.txt translators.txt; do
	cp ./$i $DOCDIR/
done

# icon
cp ./icons/32x32/deadbeef.png $OUTDIR/

# strip
if [ $OSTYPE != 'Darwin' ];then
	strip --strip-unneeded ./deadbeef-$VERSION-portable/deadbeef
	for i in $PLUGDIR/*.so ; do strip --strip-unneeded $i ; done
fi
