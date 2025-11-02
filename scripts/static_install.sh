#!/bin/bash
VERSION=$(<"build_data/VERSION")
ARCH=$(uname -m)
DESTDIR=$(pwd)/static/$ARCH/deadbeef-$VERSION make install
