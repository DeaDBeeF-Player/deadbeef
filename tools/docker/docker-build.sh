#!/bin/sh

docker build -t deadbeef -f tools/docker/Dockerfile . || exit 1
mkdir -p docker-artifacts
docker run --rm -v ${PWD}/docker-artifacts:/usr/src/deadbeef/portable deadbeef || exit 1
