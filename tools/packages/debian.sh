#!/bin/bash

set -e

PWD=$(pwd)
VERSION=$(<"build_data/VERSION")
PACKAGE_VERSION=$(echo -n $VERSION | sed 's/-/~/')
VERSION_SUFFIX=$(<"build_data/VERSION_SUFFIX")
if [[ "$ARCH" == "i686" ]]; then
    PACKAGE_ARCH=i386
elif [[ "$ARCH" == "x86_64" ]]; then
    PACKAGE_ARCH=amd64
else
    echo Unknown arch $ARCH
    exit 1
fi
INDIR=$PWD/static/$ARCH/deadbeef-${VERSION}
TEMPDIR=$PWD/package_temp/$ARCH/debian-${VERSION}
OUTDIR=$PWD/package_out/$ARCH/debian

# make dirs
rm -rf $TEMPDIR
mkdir -p $TEMPDIR
mkdir -p $OUTDIR

# copy files
cp -r $INDIR/* $TEMPDIR/
# rm unneeded files
rm -f $TEMPDIR/opt/deadbeef/lib/deadbeef/*.la

find "$TEMPDIR/opt/deadbeef/lib/deadbeef" -type f -name '*.so.0.0.0' | while IFS= read -r i; do
    n="${i%.0.0.0}"
    mv "$i" "$n"
    strip --strip-unneeded "$n"
done

strip --strip-unneeded $TEMPDIR/opt/deadbeef/bin/deadbeef

rm -f $TEMPDIR/opt/deadbeef/lib/deadbeef/*.so.*
rm -f $TEMPDIR/opt/deadbeef/lib/deadbeef/*.a

# move icons and other shit to /usr
mkdir -p $TEMPDIR/usr/share/
mv $TEMPDIR/opt/deadbeef/share/applications $TEMPDIR/usr/share/
sed -i 's/Exec=deadbeef/Exec=\/opt\/deadbeef\/bin\/deadbeef/g' $TEMPDIR/usr/share/applications/deadbeef.desktop
sed -i 's/Exec=deadbeef/Exec=\/opt\/deadbeef\/bin\/deadbeef/g' $TEMPDIR/usr/share/applications/deadbeef_enqueue.desktop
mv $TEMPDIR/opt/deadbeef/share/icons $TEMPDIR/usr/share/

# generate debian-binary
echo "2.0" >$TEMPDIR/debian-binary

# generate control
echo "Version: ${VERSION}-${VERSION_SUFFIX}" >$TEMPDIR/control
echo "Installed-Size: `du -sb $TEMPDIR | awk '{print int($1/1024)}'`" >>$TEMPDIR/control
echo "Architecture: ${PACKAGE_ARCH}" >>$TEMPDIR/control
cat $PWD/tools/packages/deb_control >>$TEMPDIR/control

# copy postinst and postrm
cp $PWD/tools/packages/deb_postinst $TEMPDIR/postinst
cp $PWD/tools/packages/deb_postrm $TEMPDIR/postrm

# generate md5sums
cd $TEMPDIR
rm -f $TEMPDIR/md5sums
find ./opt -type f | while read i ; do
    md5sum "$i" | sed 's/\.\///g' >>$TEMPDIR/md5sums
done
cd $PWD

# generate shlibs
pwd
rm -f $TEMPDIR/shlibs
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
fakeroot -- ar cr $OUTDIR/deadbeef-static_${PACKAGE_VERSION}-${VERSION_SUFFIX}_${PACKAGE_ARCH}.deb debian-binary control.tar.gz data.tar.gz

