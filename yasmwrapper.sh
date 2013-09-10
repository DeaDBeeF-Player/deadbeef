#!/bin/sh
ARGS=""
for i in $@ ; do
    #if [ "$i" != "-fPIC" ] && [ "$i" != "-DPIC" ]; then
    if [ "$i" != "-fPIC" -a "$i" != "-fno-common" ]; then
        ARGS="$ARGS $i"
    fi
done
echo "exec: yasm $ARGS"
yasm $ARGS
