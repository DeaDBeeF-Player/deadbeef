#!/bin/bash

shopt -s extglob

ORIGIN=$PWD
STATIC_DEPS=$ORIGIN/static-deps/lib-x86-64/lib
BUILD=testbuild

BAKDIR=$STATIC_DEPS/bak
mkdir -p $BAKDIR
mv -v $STATIC_DEPS/*c++*.so* $BAKDIR/

export DDB_TEST_SUITES=${@}
mkdir -p $BUILD
export LD_LIBRARY_PATH=$STATIC_DEPS
make V=1 -j --file=Tests.mk
code=$?

mv -v $BAKDIR/*c++*.so* $STATIC_DEPS/

exit $code
