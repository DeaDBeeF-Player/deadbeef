#!/bin/sh
VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
DESTDIR=`pwd`/static/deadbeef-$VERSION make install
