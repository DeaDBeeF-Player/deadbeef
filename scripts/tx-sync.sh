#!/bin/bash

set -e

./tools/localize/localize.pl --c-source --output ./translation/plugins.c
cd po
PATH="/usr/local/opt/gettext/bin:$PATH" make update-po
cd ..
tx push -s
tx pull -a -f

for i in po/*.po ; do
    if ! [[ $(git diff $i | grep '+msgstr') ]]; then
        echo $i unmodified
        git checkout HEAD $i
    fi
done
