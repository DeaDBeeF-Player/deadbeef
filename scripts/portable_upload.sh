#!/bin/bash

VERSION=$(<"build_data/VERSION")
BUILD=$(<"build_data/VERSION_SUFFIX")

UPLOAD_URI=waker,deadbeef@frs.sourceforge.net:/home/frs/project/d/de/deadbeef/portable/$VERSION/

scp portable_out/* $UPLOAD_URI/
scp portable_out/build/* $UPLOAD_URI/
