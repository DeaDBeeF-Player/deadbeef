#!/bin/bash

set -e

./tools/localize/localize.pl --c-source --output ./translation/plugins.c
cd po
PATH="/usr/local/opt/gettext/bin:$PATH" make update-po
cd ..
tx push -s
tx pull -a -f

while read i; do
    po=po/$i.po
    if ! [[ $(git diff $po | grep '+msgstr') ]]; then
        echo $i unmodified
        git checkout HEAD po/$i.po
    fi
done <po/LINGUAS
