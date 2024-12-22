#!/bin/sh

set -e

docker build --platform linux/amd64 --progress plain -t deadbeef-clang-unittest-18.04 -f tools/docker/18.04/Dockerfile-unittest .
mkdir -p docker-artifacts
docker run --platform linux/amd64 -i --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang-unittest-18.04
