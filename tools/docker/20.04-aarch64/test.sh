#!/bin/sh

docker build --platform linux/amd64 --progress plain -t deadbeef-test-20.04 -f tools/docker/20.04/Dockerfile-test . || exit 1
docker run --platform linux/amd64 --rm deadbeef-test-20.04 || exit 1

