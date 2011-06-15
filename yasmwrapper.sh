#!/bin/bash
ARGS=""
for i in $@ ; do
    #if [ "$i" != "-fPIC" ] && [ "$i" != "-DPIC" ]; then
    if [ "$i" != "-fPIC" ]; then
        ARGS="$ARGS $i"
    fi
done
echo "exec: yasm $ARGS"
yasm $ARGS
