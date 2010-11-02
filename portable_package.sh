#!/bin/sh

# package for distribution
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
BUILD=`cat PORTABLE_BUILD | perl -ne 'chomp and print'`

# main distro
SRCDIR=deadbeef-$VERSION-portable
PLUGDIR=$SRCDIR/plugins
DOCDIR=$SRCDIR/doc
PIXMAPDIR=$SRCDIR/pixmaps

mkdir -p portable_out 2>/dev/null
rm portable_out/* 2>/dev/null

tar jcvf portable_out/deadbeef-$VERSION-portable-build$BUILD.tar.bz2\
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


# plugins
cd $PLUGDIR

plugtable=../../../deadbeef-web/web/plugins-autogen.mkd
echo "<table><tr><th>Name</th><th>Version</th><th>Size</th><th>For Deadbeef</th><th>Description</th><th>Author(s)</th></tr>" >$plugtable

for i in *.so ; do
    plugname=`basename $i .so`
    version=""
    for fn in ../../plugins/$plugname/*.c* ; do
        VMAJOR=`cat $fn 2>/dev/null | grep -m 1 version_major | perl -pe 's/.*version_major[^\d]+([\d]+).*$/$1/' | perl -ne 'chomp and print'`
        VMINOR=`cat $fn 2>/dev/null | grep -m 1 version_minor | perl -pe 's/.*version_minor[^\d]+([\d]+).*$/$1/' | perl -ne 'chomp and print'`
        author=`cat $fn 2>/dev/null | grep -m 1 '\.author = ' | perl -pe 's/.*author = "(.*)".*/$1/' | perl -ne 'chomp and print'`
        email=`cat $fn 2>/dev/null | grep -m 1 '\.email = ' | perl -pe 's/.*\.email = "(.*)".*/$1/' | perl -ne 'chomp and print'`
        descr=`cat $fn 2>/dev/null | grep -m 1 '\.descr = ' | perl -pe 's/.*\.descr = "(.*)".*/$1/' | perl -ne 'chomp and print'`
        if [[ -n $VMAJOR ]] && [[ -n $VMINOR ]]; then
            version="$VMAJOR.$VMINOR"
            break
        fi
    done
    if [[ -n $version ]]; then
        echo "$plugname version $VMAJOR.$VMINOR"
    else
        echo "$plugname version not found"
    fi
    fname=../../portable_out/deadbeef-$VERSION-portable-$plugname-$version.tar.bz2
    tar jcvf $fname $i
    fsize=$(stat -c%s "$fname")

    # add some markdown
    echo "<tr><td><a href="http://sourceforge.net/projects/deadbeef/files/deadbeef-$VERSION-portable-$plugname-$version.tar.bz2/download">$plugname</a></td><td>$version</td><td>$fsize</td><td>$VERSION</td><td>$descr</td><td>$author ($email)</td></tr>" >>$plugtable
done
echo "</table>" >>$plugtable
cd ../../

