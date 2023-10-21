set -e

gcc -Wall -g -o test_program.exe test_program.c
objdump -D -M intel test_program.exe > test_program.dump
./test_program.exe
