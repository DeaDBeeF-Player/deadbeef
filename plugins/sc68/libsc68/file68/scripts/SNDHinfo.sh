#! /bin/sh
#

# Retrieve information from sndh file.
#
# (c) Benjamin Gerard <ben@sashipa.com>
#
# SNDHinfo.sh
#

progname="`basename "$0"`"
dsndh=""
sndh=""

function ErrorMsg()
{
    echo "${progname}:${sndh}: $@" >&2
}

function Error()
{
    ErrorMsg "$@"
    [ -n "${dsndh}" -a "${dsndh}" != "${sndh}" ] &&
    rm -f "${dsndh}"
    exit 255
}

sc68=no
time=no

# Parse arguments
while [ $# -ne 0 ]; do
    case "$1" in
	--sc68)
	    sc68=yes;;
	--time)
	    time=yes;;
	*)
	    sndh="$1";;
    esac
    shift
done

# Usage.
if [ -z "${sndh}" -o "${sndh}" = "--help" -o "${sndh}" = "-h" ]; then
    cat <<EOF
Retrieve information from sndh file.
(C) Benjamin Gerard <ben@sashipa.com>

Usage: ${progname} [Options] <sndh-file>

Options:
  --help -h  This message and exit.
  --sc68     Generate output for debug68 tool.
  --time     Use sndh time if available.

EOF
    exit 1
fi

# Test input
[ -e "${sndh}" ] ||
Error "Missing input file \"$1\""

dsndh="${sndh}"

# Test for ICE packed file
if [ "`strings "${sndh}" | head -n 1`" = "ICE!" ]; then
    ErrorMsg "ICE packed file !!"

    # Get unice68 if not set.
    [ -z "${unice}" ] && unice="`which unice68`"

    # test unice exe
    [ -z "${unice}" -o ! -x "${unice}" ] &&
    Error "Can find unice executable [${unice}]"

    tmpice="${sndh}-$$-ice"
    [ -e "${tmpice}" ] &&
    Error "Temporary file [${tmpice}] already exist. Delete it first."

    ${unice} "${sndh}" "${tmpice}"
    if [ $? -ne 0 ]; then
	rm -f "${tmpice}"
	Error "Failed to depack ICE file."
    fi
    dsndh="${tmpice}"
fi

DIRNAME="`dirname "${sndh}"`"
BASENAME="`basename "${sndh}" .SND`"
BASENAME="`basename "$BASENAME" .snd`"

# Read SNDh strings
str="`strings "${dsndh}" | head -n 20`"

# Check header
echo "$str" | grep -q "SNDH"
if [ $? -ne 0 ]; then
    ErrorMsg "Missing SNDH header in \"${dsndh}\""
    # Some file are broken !! Test COMM !
    echo "$str" | grep -q COMM
    if [ $? -ne 0 ]; then
	if [ "${sc68}" != "yes" ];then
	    echo "exit 255"
	fi
	exit 2
    fi
fi

COMM="`echo "$str" | grep COMM`"
COMM="`expr "$COMM" : '.*COMM\(.*\)' \| '???'`"

# Some file are broken !
if [ "$COMM" = "???" ]; then	
    COMM="`echo "$str" | grep COMP`"
    COMM="`expr "$COMM" : '.*COMP\(.*\)' \| '???'`"
fi
	
TITLE="`echo "$str" | grep TITL`"
TITLE="`expr "$TITLE" : '.*TITL\(.*\)' \| ${BASENAME}`"

RIPP="`echo "$str" | grep RIPP`"
RIPP="`expr "$RIPP" : '.*RIPP\(.*\)' \| '???'`"

CONV="`echo "$str" | grep CONV`"
CONV="`expr "$CONV" : '.*CONV\(.*\)' \| '???'`"

VBL="`echo "$str" | grep '!V[0-9][0-9]'`"
VBL="`expr "$VBL" : '.*!V\([0-9][0-9]\).*' \| '00'`"

FX="`echo "$str" | grep '\*\*[-DS][-DS]'`"
FX="`expr "$FX" : '.*\*\*\([-DS][-DS]\).*' \| '??'`"

TSEC="`echo "$str" | grep 'TIME[0-9][0-9][0-9][0-9]'`"
TSEC="`expr "$TSEC" : '.*TIME\([0-9][0-9][0-9][0-9]\).*' \| '0000'`"

TRKS="`echo "$str" | grep '##[0-9][0-9]'`"
TRKS="`expr "$TRKS" : '.*##\([0-9][0-9]\)' \| '01'`"

TC="`echo "$str" | grep 'TC[0-9][0-9][0-9]'`"
TC="`expr "$TC" : '.*TC\([0-9][0-9][0-9]\)' \| '000'`"

MUSMON="NO"
echo "$str" | grep -q MuMo && MUSMON="YES"

if [ "$sc68" != "yes" ];then
    echo "FILE:${sndh}"
    echo "TITLE:$TITLE"
    echo "AUTHOR:$COMM"
    echo "RIPPER:$RIPP"
    echo "CONVERTER:$CONV"
    echo "TRACKS:$TRKS"
    echo "TIME:$TSEC"
    echo "VBL:$VBL"
    echo "TIMER-C:$TC"
    echo "SFX:$FX"
    echo "MUSICMON:$MUSMON"
else
    if [ "${time}" = "yes" -a "${TRKS}" != "01" ]; then
	[ "${dsndh}" != "${sndh}" ] && rm "${dsndh}"
	echo "x"
	Error "Can't used SNDH time for multi-track file."
    fi

    cat <<EOF
ldat "${sndh}"
!= exit
a= "${COMM}"
n= "${TITLE}"
replay "sndh_ice"
!= exit
EOF

    FRQ=""
    [ "$TC" != "000" ] && FRQ="$TC"
    if [ "$VBL" != "00" ]; then
	if [ -n "$FRQ" ]; then
	    ErrorMsg "VBL($VBL) and Timer-C($TC) detected! Use Timer-C($FRQ)."
	else
	    [ "$VBL" != "50" ] && FRQ="$VBL"
	fi
    fi

    if [ -n "$FRQ" ]; then
	echo "frq $FRQ"
    fi
    
    [ ${time} != "yes" ] && echo "ft 1"
    [ ${time} = "yes" -a ${TSEC} -ne 0 ] &&
    printf "st 1 %02d:%02d\n" `expr $TSEC '/' 100` `expr $TSEC '%' 100`
    N=1
    while [ $N -lt $TRKS ]; do
	echo "+ $N"
	N=`expr $N + 1`
	echo "ft $N"
    done
    cat <<EOF
cap all
sd "${DIRNAME}/${BASENAME}.sc68"
exit
EOF
fi

ret=$?
[ "${dsndh}" != "${sndh}" ] && rm "${dsndh}"
exit $ret
