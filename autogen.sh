#!/bin/bash
set -e
echo "Running autopoint..."; autopoint -f
echo "Running aclocal..."; aclocal $ACLOCAL_FLAGS -I m4
echo "Running autoheader..."; autoheader
echo "Running autoconf..."; autoconf
echo "Running libtoolize..."; (libtoolize --copy --automake || glibtoolize --automake)
echo "Running automake..."; automake --add-missing --copy --gnu
echo "Running intltoolize"; intltoolize --force --automake
