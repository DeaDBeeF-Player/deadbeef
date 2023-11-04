#! /bin/sh
#
# Get musmon2 data file from .sc68 sndh file.
#
# That's a little tricky ! Don't ask me why I wrote this script file
# I just can remember :)
#
# (c) Benjamin Gerard <ben@sashipa.com>
#

if [ $# -ne 1 ]; then
    echo "usage: `basename "$0"` <musmon-sndh.sc68>"
    exit 1
fi
	
sc68="$1"
info68 "$sc68" || exit $?
mm2="`info68 "$sc68" -N`".mm2

out="`echo -e "db\nt\nt\nd\nx\nx\n" | debug68 "$1"`"
ending="`echo "$out" | grep 'LEA $....(PC),A5' | head -n 1`"
ending=`expr "$ending" : '.*LEA $\(....\)(PC),A5.*' \| '0000'`
start="`echo "$out" | grep 'LEA $....(PC),A0' | head -n 1`"
start=`expr "$start" : '.*LEA $\(....\)(PC),A0.*' \| '0000'`

echo "start=$start"
echo "ending=$ending"
	
#	[ "$start" != "0000" ] &&
#	[ "$ending" != "0000" ] &&
#	echo -e "db\nsbin \"${mm2}\" \$${start} \$${ending}-\$${start}\nx\nldat \"${mm2}\nsd \"${mm2}.sc68\nx\n" | debug68 "$sc68"
	
		
