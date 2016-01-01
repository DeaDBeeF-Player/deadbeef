#! /bin/sh
#
# unice functions
#
# (C) Benjamin Gerard <ben@sashipa.com>
#
# $Id: unice.sh 539 2015-03-11 19:16:54Z benjihan $
#

[ "${UNICE_SH_LOADED}" = "1" ] && return 0

# Check for unice68 executable
function which_unice()
{
    if [ -z "${UNICE}" ]; then
	UNICE="`which unice68 2> /dev/null`"
    fi
    if [ -z "${UNICE}" -o ! -x "${UNICE}" ]; then
	echo "unice.sh: unice68 not found" >&2
	return 255
    fi
    return 0
}

# Check if a file is ICE packed
function is_ice_packed()
{
    local header="`dd bs=1 if="$1" count=4 2> /dev/null`"
    [ "${header}" = "ICE!" ]
}

# Check and run UNICE
function unice()
{
    local dst
    which_unice || return $?
    "${UNICE}" "$@"
}


UNICE_SH_LOADED=1
#echo "unice.sh: loaded" >&2
which_unice
