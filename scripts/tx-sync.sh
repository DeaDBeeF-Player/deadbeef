#!/bin/sh

./tools/localize/localize.pl --c-source --output ./translation/plugins.c
cd po
PATH="/usr/local/opt/gettext/bin:$PATH" make update-po
cd ..
tx push -s
tx pull -a -f --no-interactive
