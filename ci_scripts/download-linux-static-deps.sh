#!/bin/bash

set -e

if [ ! -d static-deps ]; then
    rm -rf static-deps
    STATICDEPS_URL="http://sourceforge.net/projects/deadbeef/files/staticdeps/ddb-static-deps-latest.tar.bz2/download"
    mkdir static-deps
    echo "Downloading static deps..."
    wget "$STATICDEPS_URL" -O ddb-static-deps.tar.bz2
    echo "Unpacking static deps..."
    tar jxf ddb-static-deps.tar.bz2 -C static-deps
fi

