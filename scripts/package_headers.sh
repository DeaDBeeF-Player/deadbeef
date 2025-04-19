#!/bin/bash

# Create ddb-headers-latest.tar.bz2 in the current folder

mkdir -p /tmp/deadbeef-headers/deadbeef
cp include/deadbeef/deadbeef.h plugins/gtkui/gtkui_api.h plugins/artwork/artwork.h /tmp/deadbeef-headers/deadbeef/
cd /tmp/deadbeef-headers
tar jcvf ddb-headers-latest.tar.bz2 deadbeef
cd -
pwd
mv /tmp/deadbeef-headers/ddb-headers-latest.tar.bz2 ./
rm -rf /tmp/deadbeef-headers
echo Ready: ddb-headers-latest.tar.bz2
