#!/bin/sh

VERSION=`cat PORTABLE_VERSION | perl -ne 'chomp and print'`
BUILD=`cat PORTABLE_BUILD | perl -ne 'chomp and print'`

UPLOAD_URI=waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/portable/$VERSION/

scp portable_out/* $UPLOAD_URI/
scp portable_out/build/* $UPLOAD_URI/
