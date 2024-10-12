#!/bin/sh

docker build --progress plain -t deadbeef-test-20.04 -f tools/docker/20.04/Dockerfile-test . || exit 1
docker run --rm deadbeef-test-20.04 || exit 1

