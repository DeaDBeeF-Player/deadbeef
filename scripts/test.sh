#!/bin/bash

shopt -s extglob

ORIGIN=$PWD

ARCH=$(uname -m)

if [[ "$ARCH" == "x86_64" ]]; then
    STATIC_DEPS=$ORIGIN/static-deps/lib-x86-64
elif [[ "$ARCH" == "arm64" || "$ARCH" == "aarch64" ]]; then
    STATIC_DEPS=$ORIGIN/static-deps/lib-aarch64
else
    echo "Unsupported arch: $ARCH"
    exit 1
fi

BUILD=testbuild

BAKDIR=$STATIC_DEPS/lib/bak
mkdir -p $BAKDIR
mv -v $STATIC_DEPS/lib/*c++*.so* $BAKDIR/

export DDB_TEST_SUITES=${@}
mkdir -p $BUILD
export LD_LIBRARY_PATH=$STATIC_DEPS/lib
make V=1 -j --file=Tests.mk STATIC_DEPS=$STATIC_DEPS
code=$?

mv -v $BAKDIR/*c++*.so* $STATIC_DEPS/lib

exit $code
