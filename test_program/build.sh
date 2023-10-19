set -e

nasm -o test.bin -f bin test.asm
nasm -o test.o -l test.lst -f elf64 test.asm
ld -o test.exe test.o
./test.exe
