#!/bin/sh

docker build --progress plain -t deadbeef-clang-18.04 -f tools/docker/18.04/Dockerfile . || exit 1
mkdir -p docker-artifacts
docker run --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang-18.04 || exit 1
