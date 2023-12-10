set -e

gcc -Wall -g -o test.so \
    -nostartfiles -nodefaultlibs \
    -fPIC \
    -shared \
    -masm=intel \
    -fno-stack-protector \
    test_lib.c

gcc -Wall -g -o test.exe \
    -nostartfiles -nodefaultlibs \
    -masm=intel \
    -fno-stack-protector \
    test_program.c ./test.so
cp test.exe test_cpy.exe
objdump -D -M intel test.exe > test.dump
