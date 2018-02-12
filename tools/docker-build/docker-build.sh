#!/bin/sh

docker build -t deadbeef -f tools/docker-build/Dockerfile .
docker run deadbeef
