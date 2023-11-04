#! /bin/sh
#
# (c) Benjamin Gerard <ben@sashipa.com>
#
# Build SidSoundDesigner source music file from .tri and . tvs 
#
# $Id: SSDtunes.sh 539 2015-03-11 19:16:54Z benjihan $
#

NAME=""
HELP=0
timestr="= ft all"

while [ $# -ne 0 ]; do
    case "$1" in
	--help)
	    HELP=1
	    ;;
	--no-time)
	    timestr=""
	    ;;
	*)
	    if [ -n "${NAME}" ]; then
		HELP=1
	    else
		NAME="$1"
	    fi
	    ;;
    esac
    shift
done

if [ ${HELP} -ne 0 ]; then
    cat<<EOF
Build SidSoundDesigner music for sc68 ssd1 external replay.
(C) Benjamin Gerard <ben@sashipa.com>

Usage: `basename "$0"` [--no-time] <music-file-name>

EOF
    exit 1
fi

cd `dirname "${NAME}"` || exit 1
BASE=`basename "${NAME}"`
BASE="`expr "${BASE}" : '\(.*\)[.].*' \| "${BASE}"`"
TRI1="${BASE}.tri"
TRI2="${BASE}.TRI"
TVS1="${BASE}.tvs"
TVS2="${BASE}.TVS"
ASM="${BASE}.s"
BIN="${BASE}.bin"
SC68="${BASE}.sc68"

TRI=""
TVS=""
[ -e "$TRI1" ] && TRI="$TRI1"
[ -z "$TRI" -a -e "$TRI2" ]  && TRI="$TRI2"
[ -e "$TVS1" ] && TVS="$TVS1"
[ -z "$TVS" -a -e "$TVS2" ]  && TVS="$TVS2"

if [ -z "$TRI" ]; then
    echo "Missing file \"$TRI1\" or \"$TRI2\"" >&2
    exit 2
fi

if [ ! -e "$TVS" ]; then
    echo "Missing file \"$TVS1\" or \"$TVS2\"" >&2
    exit 3
fi

cat <<EOF > "${ASM}"
start:
  dc.l  'SSD1'
  dc.l  tvi-tvs
  dc.l  last-tvi
tvs:  incbin  "$TVS"
even
  dc.w  0
tvi:  incbin  "$TRI"
  even
  dc.l  'ssd1'
last:  dc.w  0
EOF

as68 "$ASM" &&
cat <<EOF | debug68
ldat "$BIN"
= replay ssd1
$timestr
= sd "${SC68}"
exit
EOF
ret=$?
rm -f "$ASM" "$BIN" 
exit ${ret}
