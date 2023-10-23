set -e

gcc -Wall -g -o test.exe test_program.c
objdump -D -M intel test.exe > test.dump
./test.exe
