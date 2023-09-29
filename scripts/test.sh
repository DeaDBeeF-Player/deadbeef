#!/bin/bash

shopt -s extglob

ORIGIN=$PWD
STATIC_DEPS=$ORIGIN/static-deps/lib-x86-64/lib
BUILD=testbuild

BAKDIR=$STATIC_DEPS/bak
mkdir -p $BAKDIR
mv -vt $BAKDIR $STATIC_DEPS/*c++*.so*

export DDB_TEST_SUITES=${@}
mkdir -p $BUILD
export LD_LIBRARY_PATH=$STATIC_DEPS
make -j --file=Tests.mk
code=$?

mv -vt $STATIC_DEPS $BAKDIR/*c++*.so*

exit $code
