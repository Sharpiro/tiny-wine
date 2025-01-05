#! /bin/bash

assert() {
    if ! "$@"; then
        echo "Assertion failed: $@" >&2 
        echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
        exit 1
    fi
}

# Dynamic Test
expected=`cat <<EOF
16 + 16 = 32
EOF
`
assert [ "`./winloader ./windynamic.exe`" == "$expected" ]

echo All tests passed
