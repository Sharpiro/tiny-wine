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
exe_global_var_bss: 0
exe_global_var_bss: 1
exe_global_var_data: 42
exe_global_var_data: 24
EOF
`
assert [ "`./winloader ./windynamic.exe`" == "$expected" ]

echo All tests passed
