#! /bin/bash

assert() {
    if ! "$@"; then
        echo "Assertion failed: $@" >&2 
        echo "${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
        exit 1
    fi
}

pushd build

# Unit Test
./unit_test
assert [ $? == 0 ]

# Env Test
assert [ "`./linloader ./env | wc -c`" == "8" ]
assert [ "`./linloader ./env x`" == $'2\n./env\nx' ]

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
assert [ "`./linloader string`" == "$expected" ]
assert [ "`./linloader string_pie`" == "$expected" ]

# Dynamic Test
expected=`cat <<EOF
1st call
2nd call
16 + 16 = 32
2nd shared lib length of 'how now brown cow': 17
dynamic_var_data: 12345
dynamic_var_data: 54321
dynamic_var_bss: 0
dynamic_var_bss: 54321
get_test_number_data_internal_ref: 12345
get_test_number_data_internal_ref: 54321
malloc: ok
add_many_result: 36
EOF
`
assert [ "`./linloader ./dynamic`" == "$expected" ]
assert [ "`./linloader ./dynamic_pie`" == "$expected" ]

result=`./linloader ./tinyfetch`
assert [ $? == 0 ]

expected="Hello Linux statically linked Tiny Wine!"
assert [ "`./linloader ./linux_static_hello_printf`" == "$expected" ]

expected="Hello Linux dynamically linked Tiny Wine!"
assert [ "`./linloader ./linux_dyn_hello_printf`" == "$expected" ]

echo All tests passed

popd
