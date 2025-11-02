#!/bin/bash

set -e

PWD=$(pwd)
VERSION=$(<"build_data/VERSION")
PACKAGE_VERSION=$(echo -n $VERSION | sed 's/-//')
VERSION_SUFFIX=$(<"build_data/VERSION_SUFFIX")
if [[ "$ARCH" == "i686" ]]; then
    echo
elif [[ "$ARCH" == "x86_64" ]]; then
    echo
else
    echo unknown arch $ARCH
    exit -1
fi
INDIR=$PWD/static/$ARCH/deadbeef-$VERSION
TEMPDIR=$PWD/package_temp/$ARCH/arch-$VERSION
PKGINFO=$TEMPDIR/.PKGINFO
INSTALL=$TEMPDIR/.INSTALL
OUTDIR=$PWD/package_out/$ARCH/arch

# make dirs
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
mv $TEMPDIR/opt/deadbeef/share/icons $TEMPDIR/usr/share/

# generate .PKGINFO
echo "# `date -u`" >$PKGINFO
echo "pkgver = $PACKAGE_VERSION-$VERSION_SUFFIX" >>$PKGINFO
echo "builddate = `date --utc  +%s`" >>$PKGINFO
echo "size = `du -sb $TEMPDIR | awk '{print $1}'`" >>$PKGINFO
echo "arch = $ARCH" >>$PKGINFO
cat tools/packages/arch_pkginfo >>$PKGINFO

# generate .INSTALL
cp tools/packages/arch_install $INSTALL

# archive
cd $TEMPDIR
chmod -R 755 .
fakeroot -- tar Jcvf $OUTDIR/deadbeef-static-${PACKAGE_VERSION}-${VERSION_SUFFIX}-${ARCH}.pkg.tar.xz * .PKGINFO .INSTALL
