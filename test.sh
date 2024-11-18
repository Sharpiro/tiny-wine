#! /bin/bash

assert() {
    if ! "$@"; then
        echo "Assertion failed: $@" >&2 
        echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
        exit 1
    fi
}

# Unit Test
$PRELOADER ./unit_test
assert [ $? == 0 ]

# Env Test
assert [ "`$PRELOADER ./loader ./env | wc -c`" == "8" ]
assert [ "`$PRELOADER ./loader ./env x`" == $'2\n./env\nx' ]

# String Test
expected=`cat <<EOF
inline string
CONST_STRING const string
CONST_ZERO 0
CONST_ONE 1
data_int 1
data_string string
bss_int 0
bss_string (null)
malloc_string abcd
lib test_number_data: 12345
lib test_number_data: 54321
lib test_number_bss: 0
lib test_number_bss: 1
lib get_test_number_data: 54321
EOF
`
assert [ "`$PRELOADER ./loader string silent`" == "$expected" ]

# Dynamic Test
expected=`cat <<EOF
1st call
2nd call
16 + 16 = 32
dynamic_var: 0
dynamic_var: 42
2nd shared lib length of 'how now brown cow': 17
lib test_number_data: 0x00012345
lib test_number_data: 0x00054321
lib test_number_bss: 0x00000000
lib test_number_bss: 0x00054321
lib get_test_number_data_internal_ref: 0x00012345
EOF
`
assert [ "`$PRELOADER ./loader ./dynamic`" == "$expected" ]

echo All tests passed
