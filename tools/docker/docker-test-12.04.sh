#!/bin/sh

docker build -t deadbeef-test-12.04 -f tools/docker/Dockerfile-test-12.04 . || exit 1
docker run --rm deadbeef-test-12.04 || exit 1

