#! /bin/bash

assert() {
    if ! "$@"; then
        echo "Assertion failed: $@" >&2 
        echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
        exit 1
    fi
}

# Env Test
assert [ `./loader ./env silent | wc -c` == "24" ]
assert [ "`./loader ./env silent`" == $'0x00000002\n./env\nsilent' ]

# String Test

expected=`cat <<EOF
inline string
const string
static string
const zero 0x00000000
static zero 0x00000000
const one 0x00000001
static one 0x00000001
EOF
`
assert [ "`./loader string silent`" == "$expected" ]

echo All tests passed
