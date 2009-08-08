#!/bin/bash
echo "IMPORTANT: Hit C-c if you forgot to backup/commit stuff!!!"
read
ls *.c *.h *.cpp | while read i; do
    if grep --silent "DO NOT EDIT" "$i" || grep --silent "DeaDBeeF - ultimate music player for GNU/Linux systems with X11" "$i"  ; then
        echo "skipping file $i"
    else
        echo "adding license text to $i"
        cat shortlicense >/tmp/deadbeef.lic.tmp
        cat "$i" >>/tmp/deadbeef.lic.tmp
        mv /tmp/deadbeef.lic.tmp "$i"
    fi
done
