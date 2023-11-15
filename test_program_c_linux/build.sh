set -e

gcc -Wall -g -o test.exe \
    -static \
    -fno-stack-protector \
    test_program.c

objdump -d -M intel test.exe > test.dump
# ./test.exe
