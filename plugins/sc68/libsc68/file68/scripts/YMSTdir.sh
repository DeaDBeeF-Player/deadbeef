#! /bin/sh
#
# (c) Benjamin Gerard
#
# Once again a old script. I can't remember it is purpose...
# Anyway it looks really obsolete to me ;)
#


if [ ! -d "$1" ]; then
    echo "Missing directory \"$1\""
    exit 1
fi

cd "$1"


for i in YM.*_replay; do
    if [ -e "$i" ]; then
	replay=`expr "$i" : 'YM.\(.*\)_replay' \| ""`
	echo "Processing replay : \"$replay\""
	if [ "$replay" != "" ]; then
	    mkdir -p replay
	    replay=`echo "$replay" | tr [:upper:] [:lower:]`
	    putYMhd "$i" "replay/${replay}"
	    if [ "$SC68PATH" != "" ]; then
		cp -i -v "replay/${replay}" "${SC68PATH}/${replay}"
	    fi
	    if [ -d "/Program Files/Winamp/Plugins/sc68/Replay/" ]; then
		cp -i -v "replay/${replay}" "/Program Files/Winamp/Plugins/sc68/Replay/${replay}"
	    fi
	fi
    fi		
done

for i in YM.* ; do
    nude=`expr "$i" : 'YM.\(.*\)' \| "$i"`
    replay=`expr "$nude" : '\(.*\)_replay' \| ""`
    data="${nude}.bin"
    if [ "$replay" = "" ]; then
	YMINFO=`YMSTinfo "$i"`
	echo "$YMINFO"
	putYMhd "$i" "$data"
	echo "$YMINFO" | debug68
		# | grep -G -v '^findtime' 
	rm -f "$data"
    fi
done

for i in *.sc68; do
    if [ -e "$i" ]; then
	name="`info68 "$i" -N`".sc68
	name="`echo "$name" | tr '?' '-' | tr '/' '-'`" 
	if [ "$name" != '.sc68' ]; then
	    mkdir -p sc68
	    mv -v "$i" sc68/"$name"
	fi
    fi
done


