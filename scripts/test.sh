#!/bin/bash

set -e
shopt -s extglob

BUILD=testbuild
TEST_C_SOURCES="*.c shared/*.c scriptable/*.c plugins/libparser/*.c plugins/nullout/*.c ConvertUTF/*.c metadata/*.c plugins/m3u/*.c plugins/vfs_curl/*.c plugins/shellexec/*.c external/mp4p/src/*.c external/wcwidth/*.c md5/*.c plugins/mp3/*.c Tests/*.c"
TEST_CPP_SOURCES="Tests/*.cpp"
GOOGLE_TEST_SOURCES="external/googletest/googletest/src/gtest-all.cc"
ORIGIN=$PWD
STATIC_DEPS=static-deps
INCLUDE="-I external/googletest/googletest -I external/googletest/googletest/include -I external/mp4p/include -I plugins/libparser -I shared -I . -I ConvertUTF -I$ORIGIN/$STATIC_DEPS/lib-x86-64/include"
LDFLAGS="-L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib -L$ORIGIN/$STATIC_DEPS/lib-x86-64/lib/x86_64-linux-gnu"
LIBRARIES="-lmad -lmpg123 -lcurl -ldispatch -lpthread -lBlocksRuntime -lm -ljansson -ldl"
export LD_LIBRARY_PATH="$ORIGIN/$STATIC_DEPS/lib-x86-64/lib"

CC="${CC:-clang}"
CXX="${CXX:-clang++}"

CFLAGS="-fblocks -O3 -D_FORTIFY_SOURCE=0 -D_GNU_SOURCE $INCLUDE -DHAVE_LOG2=1 -DDOCDIR=\"\" -DPREFIX=\"\" -DLIBDIR=\"\" -DVERSION=\"\" -DUSE_LIBMAD -DUSE_LIBMPG123 -DXCTEST -DGOOGLETEST_STATIC"

# hack: move c++ runtime out of the lib folder, to allow linking test runner to
# the newer c++ runtime
rm -rf "$BUILD"
mkdir -p "$BUILD"

mkdir -p "$BUILD/cpplibs"
mv $ORIGIN/$STATIC_DEPS/lib-x86-64/lib/*c++*.so* "$BUILD/cpplibs/"

for file in $TEST_C_SOURCES; do
    base=$(basename $file)
    if [ "$base" = "main.c" ]; then
        continue
    fi
    echo $CC -std=c99 $CFLAGS -c "$file" -o "$BUILD/${base%.@(c|cc|cpp)}.o"
    $CC -std=c99 $CFLAGS -c "$file" -o "$BUILD/${base%.@(c|cc|cpp)}.o"
done

for file in $GOOGLE_TEST_SOURCES $TEST_CPP_SOURCES; do
    base=$(basename $file)
    echo $CXX -std=c++14 $CFLAGS -c "$file" -o "$BUILD/${base%.@(c|cc|cpp)}.o"
    $CXX -std=c++14 $CFLAGS -c "$file" -o "$BUILD/${base%.@(c|cc|cpp)}.o"
done

$CXX $LDFLAGS $BUILD/*.o $LIBRARIES -o "$BUILD/runtests"
"$BUILD/runtests"

# restore cpp libs
mv "$BUILD"/cpplibs/* $ORIGIN/$STATIC_DEPS/lib-x86-64/lib/
