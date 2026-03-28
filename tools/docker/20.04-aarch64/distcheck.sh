#!/bin/sh

set -e

docker build --platform linux/amd64 --progress plain -t deadbeef-clang-distcheck-20.04 -f tools/docker/20.04/Dockerfile-distcheck .
mkdir -p docker-artifacts
docker run --platform linux/amd64 -i --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef-clang-distcheck-20.04
