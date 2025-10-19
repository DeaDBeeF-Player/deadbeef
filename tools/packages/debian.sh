#!/bin/bash

PWD=`pwd`
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
DEB_VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print' | sed 's/-/~/'`
BUILD=`cat PORTABLE_BUILD | perl -ne 'chomp and print'`
if [[ "$ARCH" == "i686" ]]; then
    DEB_ARCH=i386
elif [[ "$ARCH" == "x86_64" ]]; then
    DEB_ARCH=amd64
else
    echo unknown arch $ARCH
    exit -1
fi
INDIR=$PWD/static/$ARCH/deadbeef-$VERSION
TEMPDIR=$PWD/package_temp/$ARCH/debian-$VERSION
OUTDIR=$PWD/package_out/$ARCH/debian

# make dirs
rm -rf $TEMPDIR
mkdir -p $TEMPDIR
mkdir -p $OUTDIR

# copy files
cp -r $INDIR/* $TEMPDIR/
# rm unneeded files
rm $TEMPDIR/opt/deadbeef/lib/deadbeef/*.la
for i in $TEMPDIR/opt/deadbeef/lib/deadbeef/*.so.0.0.0; do
    n=$TEMPDIR/opt/deadbeef/lib/deadbeef/`basename $i .0.0.0`
    mv $i $n
    strip --strip-unneeded $n
done
strip --strip-unneeded $TEMPDIR/opt/deadbeef/bin/deadbeef

rm $TEMPDIR/opt/deadbeef/lib/deadbeef/*.so.*
rm $TEMPDIR/opt/deadbeef/lib/deadbeef/*.a

# move icons and other shit to /usr
mkdir -p $TEMPDIR/usr/share/
mv $TEMPDIR/opt/deadbeef/share/applications $TEMPDIR/usr/share/
sed -i 's/Exec=deadbeef/Exec=\/opt\/deadbeef\/bin\/deadbeef/g' $TEMPDIR/usr/share/applications/deadbeef.desktop
sed -i 's/Exec=deadbeef/Exec=\/opt\/deadbeef\/bin\/deadbeef/g' $TEMPDIR/usr/share/applications/deadbeef_enqueue.desktop
mv $TEMPDIR/opt/deadbeef/share/icons $TEMPDIR/usr/share/

# generate debian-binary
echo "2.0" >$TEMPDIR/debian-binary

# generate control
echo "Version: $VERSION-$BUILD" >$TEMPDIR/control
echo "Installed-Size: `du -sb $TEMPDIR | awk '{print int($1/1024)}'`" >>$TEMPDIR/control
echo "Architecture: $DEB_ARCH" >>$TEMPDIR/control
cat $PWD/tools/packages/deb_control >>$TEMPDIR/control

# copy postinst and postrm
cp $PWD/tools/packages/deb_postinst $TEMPDIR/postinst
cp $PWD/tools/packages/deb_postrm $TEMPDIR/postrm

# generate md5sums
cd $TEMPDIR
rm $TEMPDIR/md5sums
find ./opt -type f | while read i ; do
    md5sum "$i" | sed 's/\.\///g' >>$TEMPDIR/md5sums
done
cd $PWD

# generate shlibs
pwd
rm $TEMPDIR/shlibs
ls $TEMPDIR/opt/deadbeef/lib/deadbeef/*.so | while read i ; do
echo "`basename $i .so` 0 deadbeef" >>shlibs
done

# archive control
cd $TEMPDIR
chmod 644 control md5sums shlibs
chmod 755 postrm postinst
fakeroot -- tar zcvf ./control.tar.gz ./control ./md5sums ./postrm ./postinst ./shlibs
# archive data
fakeroot -- tar zcvf ./data.tar.gz ./opt ./usr

# make final archive
fakeroot -- ar cr $OUTDIR/deadbeef-static_${DEB_VERSION}-${BUILD}_$DEB_ARCH.deb debian-binary control.tar.gz data.tar.gz

