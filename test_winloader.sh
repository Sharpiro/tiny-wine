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
pow: 16
pow rdi, rsi: 42, 43
exe_global_var_bss: 0
exe_global_var_bss: 1
exe_global_var_data: 42
exe_global_var_data: 24

*get_lib_var_bss(): 0
lib_var_bss: 0
lib_var_bss: 1
lib_var_bss: 44
*get_lib_var_bss(): 44
*get_lib_var_data(): 42
lib_var_data: 42
lib_var_data: 43
lib_var_data: 44
*get_lib_var_data(): 44
add_many_msvcrt: 36
add_many_msvcrt rdi, rsi: 42, 43
EOF
`

result=`./winloader ./windynamic.exe`
assert [ $? == 0 ]
assert [ "$result" == "$expected" ]

# Dynamic stdlib test

expected=`cat <<EOF
look how far we've come
malloc: ffffffff
stdin: 0, stdout: 1, stderr: 2
EOF
`
result=`./winloader ./windynamicfull.exe`
assert [ $? == 0 ]
assert [ "$result" == "$expected" ]

echo All tests passed
