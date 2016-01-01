#! /bin/sh
#
# Make quartet sc68 file.
#
# (C) Benjamin Gerard <ben@sashipa.com>
#

progname="`basename "$0"`"

# Error output
function Error()
{
    echo "${progname}: $@" >&2
    exit 255
}

function ErrorMsg()
{
    echo "${progname}: $@" >&2
}


# Check parms
if [ $# -lt 3 ]; then
    echo "Usage: ${progname} <dest.sc68> <file.set> <song.4v [song2.4v ...]>"
    exit 1
fi

# Check tools
[ -z "${debug68}" ] && debug68="`which debug68`"
[ -z "${debug68}" -o ! -x  "${debug68}" ] &&
Error "Missing debug68 executable [${debug68}]"

[ -z "${as68}" ] && as68="`which as68`"
[ -z "${as68}" -o ! -x  "${as68}" ] &&
Error "Missing as68 executable [${as68}]"

# Build as68 source file.
function ProcessAsm
{
    local incbin=""
    local voiceset
    local N=0

    if [ ! -e "$1" ]; then
	ErrorMsg "Missing voice set file \"$1\""
	return 255
    fi
    voiceset="$1"
    shift
	
    cat<<EOF
quartet_file:
  dc.l  'QUAR'
  dc.l  voiceset-quartet_file
  dc.l  (last_song-first_song)/4
first_song:
EOF

    while [ $# -ne 0 ]; do
	if [ -e "$1" ]; then
	    echo "  dc.l	song${N}-quartet_file"
	    incbin="${incbin}song${N}:\n  incbin \"$1\"\n  even\n"
	    N=`expr ${N} + 1`
	fi
	shift
    done
    echo -e "last_song:\nvoiceset:\n  incbin \"$voiceset\"\n  even\n"
    echo -e "${incbin}"
}

dest="$1"
shift
ProcessAsm $* > "${dest}.s"
if [ $? -eq 0]; then
    ${as68} "${dest}.s" &&
    echo -e "ldat \"${dest}.bin\"\nfrq 200\nreplay quartet\nsd \"${dest}\"\nx\n" |
    ${debug68}
fi
ret=$?
rm -fv "${dest}.s" "${dest}.bin"
exit ${ret}
