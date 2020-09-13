#!/bin/sh

# run this script once to create the base container image,
# which would contain all necessary dependencies and build tools

docker build -f tools/docker/Dockerfile-builder-clang -t deadbeef-builder-clang-player .
