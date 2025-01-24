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
exe_global_var_bss: 0x0
exe_global_var_bss: 0x1
exe_global_var_data: 0x42
exe_global_var_data: 0x24

*get_lib_var_bss(): 0x0
lib_var_bss: 0x0
lib_var_bss: 0x1
lib_var_bss: 0x44
*get_lib_var_bss(): 0x44
*get_lib_var_data(): 0x42
lib_var_data: 0x42
lib_var_data: 0x43
lib_var_data: 0x44
*get_lib_var_data(): 0x44
EOF
`
assert [ "`./winloader ./windynamic.exe`" == "$expected" ]

echo All tests passed
