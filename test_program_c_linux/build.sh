set -e

gcc -Wall -g -o test.exe \
    -static \
    -fno-stack-protector \
    test_program.c
cp test.exe test_cpy.exe
objdump -D -M intel test.exe > test.dump
# ./test.exe
