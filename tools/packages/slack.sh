#!/bin/sh
PWD=`pwd`
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ARCH_VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print' | sed 's/-//'`
BUILD=`cat PORTABLE_BUILD | perl -ne 'chomp and print'`
ARCH=`uname -m | perl -ne 'chomp and print'`
INDIR=$PWD/static/$ARCH/deadbeef-$VERSION
TEMPDIR=$PWD/package_temp/$ARCH/slackware-$VERSION
PKGINFO=$TEMPDIR/.PKGINFO
INSTALL=$TEMPDIR/.INSTALL
OUTDIR=$PWD/package_out/$ARCH/slackware

# make dirs
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
rm $TEMPDIR/opt/deadbeef/lib/deadbeef/*.so.*
rm $TEMPDIR/opt/deadbeef/lib/deadbeef/*.a

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
fakeroot -- tar zcvf $OUTDIR/deadbeef-static-$ARCH_VERSION-$BUILD-$ARCH.tgz *
