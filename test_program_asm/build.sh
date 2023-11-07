set -e

rm -rf *.o *.exe

nasm -o lib.o -l lib.lst -f elf64 lib.asm

# nasm -o test.bin -f bin test.asm
nasm -o test.o -l test.lst -f elf64 test.asm
ld -o test.exe lib.o test.o
# ld -o test.exe test.o
cp test.exe test.exe.cpy
# ./test.exe hello_sailor
