#!/bin/bash
echo "IMPORTANT: Hit C-c if you forgot to backup/commit stuff!!!"
read
ls *.c *.h *.cpp | while read i; do
    if [ "$i" == "deadbeef.h" ] || [ "$i" == "config.h" ] || grep --silent "DO NOT EDIT" "$i" ; then
        echo "skipping $i (blacklist)"
    elif grep --silent "DeaDBeeF - ultimate music player for GNU/Linux systems with X11" "$i" ; then
        if grep --silent "version 3 of the License" "$i" ; then
            echo "changing $i to gpl2"
            sed -i 's/version 3 of the License/version 2 of the License/' "$i"
        else
            echo "skipping $i (already has license)"
        fi
    else
        echo "adding license text to $i"
        cat shortlicense >/tmp/deadbeef.lic.tmp
        cat "$i" >>/tmp/deadbeef.lic.tmp
        mv /tmp/deadbeef.lic.tmp "$i"
    fi
done
