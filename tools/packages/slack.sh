#!/bin/bash

set -e

PWD=$(pwd)
VERSION=$(<"build_data/VERSION")
PACKAGE_VERSION=$(echo -n $VERSION | sed 's/-//')
VERSION_SUFFIX=$(<"build_data/VERSION_SUFFIX")
PACKAGE_ARCH=$(uname -m)
INDIR=$PWD/static/${PACKAGE_ARCH}/deadbeef-${VERSION}
TEMPDIR=$PWD/package_temp/${PACKAGE_ARCH}/slackware-${VERSION}
PKGINFO=$TEMPDIR/.PKGINFO
INSTALL=$TEMPDIR/.INSTALL
OUTDIR=$PWD/package_out/${PACKAGE_ARCH}/slackware

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

rm -f $TEMPDIR/opt/deadbeef/lib/deadbeef/*.so.*
rm -f $TEMPDIR/opt/deadbeef/lib/deadbeef/*.a

# move icons and other shit to /usr
mkdir -p $TEMPDIR/usr/share/
mv $TEMPDIR/opt/deadbeef/share/applications $TEMPDIR/usr/share/
sed -i 's/Exec=deadbeef/Exec=\/opt\/deadbeef\/bin\/deadbeef/g' $TEMPDIR/usr/share/applications/deadbeef.desktop 
mv $TEMPDIR/opt/deadbeef/share/icons $TEMPDIR/usr/share/

# doinst
mkdir -p $TEMPDIR/install
cp tools/packages/slack-doinst.sh $TEMPDIR/install/doinst.sh
cp tools/packages/slack-desc $TEMPDIR/install/

# archive
cd $TEMPDIR
fakeroot -- tar zcvf $OUTDIR/deadbeef-static-${PACKAGE_VERSION}-${VERSION_SUFFIX}-${PACKAGE_ARCH}.tgz *
