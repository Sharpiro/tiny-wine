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
'./windynamic.exe', 'a', 'b', 'c'
large params: 1, 2, 3, 4, 5, 6, 7, 8
uint32: 12345678, uint64: 1234567812345678
pow: 16
malloc: abcdef01
stdin: 0, stdout: 1, stderr: 2
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
preserved registers: 1, 2, 3, 4, 5, 6, 7, 8
EOF
`

result=`./winloader ./windynamic.exe a b c`
assert [ $? == 0 ]
assert [ "$result" == "$expected" ]

# Dynamic stdlib test

expected=`cat <<EOF
'./windynamicfull.exe', 'a', 'b', 'c'
large params: 1, 2, 3, 4, 5, 6, 7, 8
uint32: 12345678, uint64: 1234567812345678
pow: 16
malloc: abcdef01
stdin: 0, stdout: 1, stderr: 2
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
EOF
`
result=`./winloader ./windynamicfull.exe a b c`
assert [ $? == 0 ]
assert [ "$result" == "$expected" ]

echo All tests passed
