#!/bin/sh

cd po
PATH="/usr/local/opt/gettext/bin:$PATH" make update-po
cd ..
tx push -s
tx pull -a -f --no-interactive
