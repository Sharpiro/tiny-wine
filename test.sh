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
assert [ "`$PRELOADER ./loader ./env silent | wc -c`" == "24" ]
assert [ "`$PRELOADER ./loader ./env silent`" == $'0x00000002\n./env\nsilent' ]

# String Test
expected=`cat <<EOF
inline string
CONST_STRING const string
CONST_ZERO 0x00000000
CONST_ONE 0x00000001
data_int 0x00000001
data_string string
bss_int 0x00000000
bss_string 0x00000000
malloc_string abcd
EOF
`
assert [ "`$PRELOADER ./loader string silent`" == "$expected" ]

# Dynamic Test
expected=`cat <<EOF
1st call
2nd call
0x00000010 + 0x00000010 = 0x00000020
dynamic_var: 0x0000002a
shared lib: 0x0000002a
EOF
`
assert [ "`$PRELOADER ./loader ./dynamic silent`" == "$expected" ]

echo All tests passed
