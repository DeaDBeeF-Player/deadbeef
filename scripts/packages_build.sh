#!/bin/sh
ARCH=i686 ./tools/packages/arch.sh || exit -1
ARCH=i686 ./tools/packages/debian.sh || exit -1
ARCH=x86_64 ./tools/packages/arch.sh || exit -1
ARCH=x86_64 ./tools/packages/debian.sh || exit -1
