#!/bin/sh

docker build --progress plain -t deadbeef-test-18.04 -f tools/docker/18.04/Dockerfile-test . || exit 1
docker run --rm deadbeef-test-18.04 || exit 1

