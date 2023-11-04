#! /bin/sh
#
# Extract part of file.
#
# (C) Benjamin Gerard
#
# $Id: extractor.sh 539 2015-03-11 19:16:54Z benjihan $
#

[ "${EXTRACTOR_SH_LOADED}" = "1" ] && return 0

TOP="`dirname "$0"`"

if [ -z "${STROFFSET}" ]; then
    STROFFSET="`which shastroffset 2> /dev/null`"
fi

if [ -z "${STROFFSET}" -o ! -x "${STROFFSET}" ]; then
    echo "extractor.sh: shastroffset not found" >&2
    return 255
fi

source "${TOP}/unice.sh" || return $?

# Get tag position in file.
#
# $1 : filename
# $2 : start offset
# $3 : tag-name
# $4 : occurence
#
function extractor_get_tag()
{
    local oc=""
    [ -n "$4" ] && oc="--$4"
    dd bs=1 if="$1" skip=$2 2>/dev/null\
	| "${STROFFSET}" $oc "$3" /dev/stdin\
	| tail -n 1
}

# Extract part of file to stdout.
#
# $1 : filename
# $2 : start tag value
# $3 : occurence of start tag (0 means last)
# $4 : end tag value (optionnal, defaulted to EOF)
# $5 : occurence of end tag
#
# @note end tag is always searched from start position.
function extractor()
{
    local src tmp start stop
    src="$1"
    
    # Check for ice packed files
    is_ice_packed "${src}"
    if [ $? -eq 0 ]; then
	tmp="${src}-$$.tmp"
	unice "${src}" "${tmp}"
	if [ $? -ne 0 ]; then
	    rm -f "${tmp}"
	    return 255
	fi
	src="${tmp}"
    fi
    
    # Get start offset
    start="`extractor_get_tag "${src}" 0 "$2" "$3"`"
    if [ -z "${start}" ]; then
	[ -n "${tmp}" ] && rm -f "${tmp}"
	return 255
    fi
    
    # Get stop offset
    stop=""
    if [ -n "$4" ]; then
	stop="`extractor_get_tag "${src}" ${start} "$4" "$5"`"
	if [ -z "${stop}" ]; then
	    [ -n "${tmp}" ] && rm -f "${tmp}"
	    return 255
	fi
    fi
    
    if [ -z "${stop}" ]; then
	dd bs=1 if="${src}" skip=${start} 2>/dev/null
    else
	stop=`expr ${stop} - ${start}` &&
	dd bs=1 if="${src}" skip=${start} count=${stop} 2> /dev/null
    fi
    start=$?
    [ -n "${tmp}" ] && rm -f "${tmp}"
    return ${start}
}

EXTRACTOR_SH_LOADED=1
#echo "extractor.sh: loaded." >&2
