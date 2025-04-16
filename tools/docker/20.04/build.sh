#!/bin/sh

docker build --platform linux/amd64 --progress plain -t deadbeef-clang-20.04 -f tools/docker/20.04/Dockerfile . || exit 1
mkdir -p docker-artifacts
docker run --platform linux/amd64 --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang-20.04 || exit 1
