#!/bin/bash

# package for distribution
VERSION=$(<"build_data/VERSION")
BUILD=$(<"build_data/VERSION_SUFFIX")

# main distro
SRCDIR=deadbeef-$VERSION-portable
PLUGDIR=$SRCDIR/plugins
DOCDIR=$SRCDIR/doc
PIXMAPDIR=$SRCDIR/pixmaps

mkdir -p portable_out/build 2>/dev/null
rm portable_out/* 2>/dev/null
rm portable_out/build/* 2>/dev/null

tar jcvf portable_out/build/deadbeef-$VERSION-portable-r$BUILD.tar.bz2\
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
    $PLUGDIR/mp3.so\
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

# plugins
cd $PLUGDIR

plugtable=../../../deadbeef-web/web/plugins-autogen.mkd
echo "<table><tr><th>Name</th><th>Version</th><th>Size</th><th>For Deadbeef</th><th>Description</th><th>Author(s)</th></tr>" >$plugtable

PLUGINFO=../../tools/pluginfo/pluginfo

for i in *.so ; do
    plugname=`basename $i .so`
    echo $plugname

    version=""
    $PLUGINFO ./$i >./temp.sh
    RET=$?
    if [ "$RET" = "0" ]; then
        source ./temp.sh
        rm ./temp.sh
        if [[ -n $version ]]; then
            echo "$plugname version $version"
        else
            echo "$plugname version not found"
        fi
        fname=../../portable_out/deadbeef-$VERSION-portable-$plugname-$version.tar.bz2
        tar jcvf $fname $i
        fsize=$(stat -c%s "$fname")

        # add some markdown
        echo "<tr><td><a href="http://sourceforge.net/projects/deadbeef/files/portable/$VERSION/deadbeef-$VERSION-portable-$plugname-$version.tar.bz2/download">$name ($plugname)</a></td><td>$version</td><td>$fsize</td><td>$VERSION</td><td>$descr</td><td>$author ($email, $website)</td></tr>" >>$plugtable
    fi
done
echo "</table>" >>$plugtable
cd ../../

