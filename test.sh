#! /bin/bash

assert() {
    if ! "$@"; then
        echo "Assertion failed: $@" >&2 
        echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
        exit 1
    fi
}

env_result=`./env`

echo $env_result

# assert [ 1 -eq 1 ]
# assert [ 1 -eq 2 ]
