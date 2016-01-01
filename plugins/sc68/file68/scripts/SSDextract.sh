#! /bin/sh
#
# Extrack .tri and .tvs from file
#
# (C)2003 Benjamin Gerard <ben@sashipa.com>
#
# $Id: SSDextract.sh 539 2015-03-11 19:16:54Z benjihan $
#

TOP="`dirname "$0"`"
source "${TOP}/extractor.sh" || exit $?
PROGNAME="`basename "$0"`"

if [ $# -lt 1 ]; then
    echo "${PROGNAME}: missing argument" >&2
    exit 255
fi

if [ "$1" = "--help" ]; then
    cat <<EOF
Extract .tri and .tvs from file.

(C)2003 Benjamin Gerard <ben@sashipa.com>

usage: ${PROGNAME} <file>

EOF
    exit 1
fi

src="$1"
tmp=""

if [ ! -e "${src}" ]; then
    echo "${PROGNAME} : file not found [${src}]" >&2
    exit 255
fi

base="`basename "${src}"`"
path="`dirname "${src}"`"
nude="`expr "${base}" : '\(.*\)\..*' \| "${base}" `"
tri="${path}/${nude}.tri"
tvs="${path}/${nude}.tvs"

tmp=""
is_ice_packed "${src}"
if [ $? -eq 0 ]; then
    tmp="${src}-$$.tmp"
    echo "ICE! detected, using temp [${tmp}]" >&2
    unice "${src}" "${tmp}" || exit 255
    src="${tmp}"
fi

TSSS="`${STROFFSET} TSSS "${src}" | tail -n 1`"
TSST="`${STROFFSET} TSST "${src}" | tail -n 1`"

echo "TSSS=${TSSS}" >&2
echo "TSST=${TSST}" >&2

ret=0
if [ -z "${TSSS}" -o -z "${TSST}" ]; then
    echo "${PROGNAME}: Could not find TSSS and/or TSST tag" >&2
    ret=255
else
    if [ ${TSSS} -lt ${TSST} ]; then
	cnt=`expr ${TSST} -  ${TSSS}`
	dd bs=1 if="${src}" skip=${TSSS} count=${cnt} > "${tvs}" &&
	dd bs=1 if="${src}" skip=${TSST}  > "${tri}"
    else
	cnt=`expr ${TSSS} -  ${TSST}`
	dd bs=1 if="${src}" skip=${TSST} count=${cnt} > "${tri}" &&
	dd bs=1 if="${src}" skip=${TSSS}  > "${tvs}"
    fi
    if [ $? -ne 0 ]; then
	echo "${PROGNAME}: Extraction failed !" >&2
	ret=255
    fi
fi

[ -n "${tmp}" ] && rm -f "${tmp}"
[ ${ret} -ne 0 ] && rm -f "${tri}" "${tvs}"

exit $ret
