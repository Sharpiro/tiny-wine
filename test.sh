#! /bin/bash

assert() {
    if ! "$@"; then
        echo "Assertion failed: $@" >&2 
        echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
        exit 1
    fi
}

# expected=`cat <<EOF
# 0x00000002 env
# silent
# EOF
# `

# Env Test
assert [ `./loader ./env silent | wc -c` == "24" ]
assert [ "`./loader ./env silent`" == $'0x00000002\n./env\nsilent' ]

# # String Test
# assert [ "`./loader string silent`" == $'const data\nstatic data' ]

echo All tests passed
