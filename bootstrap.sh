#!/bin/sh
aclocal
libtoolize
autoconf
automake -a -c
