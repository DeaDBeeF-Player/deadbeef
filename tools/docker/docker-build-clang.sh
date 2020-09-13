#!/bin/sh

docker build -t deadbeef-clang -f tools/docker/Dockerfile-clang . || exit 1
mkdir -p docker-artifacts
docker run --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang || exit 1
