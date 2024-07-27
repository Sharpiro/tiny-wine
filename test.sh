#! /bin/bash

assert() {
    if ! "$@"; then
        echo "Assertion failed: $@" >&2 
        echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
        exit 1
    fi
}

env_result_len=`./loader ./env test | wc -c`
env_result=`./loader ./env test`
expected=`cat <<EOF
0x00000002
./env
test
EOF
`

assert [ "$env_result_len" == "22" ]
assert [ "$env_result" == "$expected" ]
