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
assert [ "`./loader string silent`" == $'const string\ninline string' ]

echo All tests passed
