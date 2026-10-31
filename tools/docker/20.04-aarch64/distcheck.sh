#!/bin/sh

set -e

docker build --platform linux/aarch64 --progress plain -t deadbeef-clang-distcheck-20.04-aarch64 -f tools/docker/20.04-aarch64/Dockerfile-distcheck .
mkdir -p docker-artifacts
docker run --platform linux/aarch64 -i --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang-distcheck-20.04-aarch64
