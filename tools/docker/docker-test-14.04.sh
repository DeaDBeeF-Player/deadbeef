#!/bin/sh

docker build -t deadbeef-test-14.04 -f tools/docker/Dockerfile-test-14.04 . || exit 1
docker run --rm deadbeef-test-14.04 || exit 1

