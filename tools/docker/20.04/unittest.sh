#!/bin/sh

set -e

docker build --progress plain -t deadbeef-clang-unittest-20.04 -f tools/docker/20.04/Dockerfile-unittest .
mkdir -p docker-artifacts
docker run -i --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang-unittest-20.04
