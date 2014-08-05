#!/bin/sh
ARCH=i686 ./scripts/static_build.sh || exit -1
ARCH=i686 ./scripts/portable_package_static.sh || exit -1
ARCH=x86_64 ./scripts/static_build.sh || exit -1
ARCH=x86_64 ./scripts/portable_package_static.sh || exit -1
