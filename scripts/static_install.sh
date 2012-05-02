#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
ARCH=`uname -m | perl -ne 'chomp and print'`
DESTDIR=`pwd`/static/$ARCH/deadbeef-$VERSION make install
