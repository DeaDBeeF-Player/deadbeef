#!/bin/sh

docker build --platform linux/amd64 --progress plain -t deadbeef-test-18.04 -f tools/docker/18.04/Dockerfile-test . || exit 1
docker run --platform linux/amd64 --rm deadbeef-test-18.04 || exit 1

