#!/bin/sh

docker build --platform linux/aarch64 --progress plain -t deadbeef-clang-20.04-aarch64 -f tools/docker/20.04-aarch64/Dockerfile . || exit 1
mkdir -p docker-artifacts
docker run --platform linux/aarch64 --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang-20.04-aarch64 || exit 1
