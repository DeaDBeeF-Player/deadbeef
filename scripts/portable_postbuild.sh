#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
OSTYPE=`uname -s`
ARCH=`uname -m | perl -ne 'chomp and print'`
OUTDIR=portable/$ARCH/deadbeef-$VERSION
PLUGDIR=$OUTDIR/plugins
DOCDIR=$OUTDIR/doc
PIXMAPDIR=$OUTDIR/pixmaps
echo OUTDIR=$OUTDIR

rm -rf $OUTDIR

mkdir -p $PLUGDIR
mkdir -p $DOCDIR
mkdir -p $PIXMAPDIR

cp ./deadbeef $OUTDIR

for i in nullout cdda flac alsa mpgmad hotkeys vtx \
	 ffap ffmpeg wavpack vorbis oss vfs_curl \
	 lastfm sid adplug sndfile artwork alac \
	 supereq gme dumb notify musepack wildmidi \
	 tta dca aac mms shn ao shellexec shellexecui vfs_zip \
	 m3u converter pulse dsp_libsrc mono2stereo ; do
    if [ -f ./plugins/$i/.libs/$i.so ]; then
		 cp ./plugins/$i/.libs/$i.so $PLUGDIR/
	elif [ -f ./plugins/$i/$i.so ]; then
		 cp ./plugins/$i/$i.so $PLUGDIR/
	elif [ -f ./plugins/$i/.libs/ddb_$i.so ]; then
		 cp ./plugins/$i/.libs/ddb_$i.so $PLUGDIR/
	else
		echo ./plugins/$i/.libs/$i.so not found
	fi

    if [ -f ./plugins/$i/.libs/${i}_gtk2.so ]; then
		 cp ./plugins/$i/.libs/${i}_gtk2.so $PLUGDIR/
    fi
    if [ -f ./plugins/$i/.libs/${i}_gtk3.so ]; then
		 cp ./plugins/$i/.libs/${i}_gtk3.so $PLUGDIR/
    fi

    if [ -f ./plugins/$i/.libs/$i.fallback.so ]; then
		 cp ./plugins/$i/.libs/$i.fallback.so $PLUGDIR/
    fi
done

if [ -f ./plugins/gtkui/.libs/ddb_gui_GTK2.so ]; then
    cp ./plugins/gtkui/.libs/ddb_gui_GTK2.so $PLUGDIR/
else
    echo ./plugins/gtkui/.libs/ddb_gui_GTK2.so not found
fi
if [ -f ./plugins/gtkui/.libs/ddb_gui_GTK2.fallback.so ]; then
    cp ./plugins/gtkui/.libs/ddb_gui_GTK2.fallback.so $PLUGDIR/
fi

if [ -f ./plugins/gtkui/.libs/ddb_gui_GTK3.so ]; then
    cp ./plugins/gtkui/.libs/ddb_gui_GTK3.so $PLUGDIR/
else
    echo ./plugins/gtkui/.libs/ddb_gui_GTK3.so not found
fi

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
cp -r plugins/converter/presets $OUTDIR/plugins/convpresets

# translations
mkdir -p $OUTDIR/locale
for i in po/*.gmo ; do
    base=`basename po/$i .gmo`
    mkdir -p $OUTDIR/locale/$base/LC_MESSAGES
    cp $i $OUTDIR/locale/$base/LC_MESSAGES/deadbeef.mo
done
cp translation/help.pt_BR.txt $OUTDIR/doc/
cp translation/help.ru.txt $OUTDIR/doc/
touch $OUTDIR/.ddb_portable

# strip
if [ $OSTYPE != 'Darwin' ];then
	strip --strip-unneeded $OUTDIR/deadbeef
	for i in $PLUGDIR/*.so ; do strip --strip-unneeded $i ; done
fi
