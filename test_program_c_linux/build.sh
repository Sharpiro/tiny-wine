set -e

gcc -Wall -g -o test.exe \
    -masm=intel \
    -fno-stack-protector \
    test_program.c
cp test.exe test_cpy.exe
objdump -D -M intel test.exe > test.dump
