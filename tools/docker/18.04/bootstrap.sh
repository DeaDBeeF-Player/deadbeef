#!/bin/sh

# run this script once to create the base container image,
# which would contain all necessary dependencies and build tools

docker build --platform linux/amd64 --progress plain -f tools/docker/18.04/Dockerfile-builder -t deadbeef-builder-player-clang-18.04 .
