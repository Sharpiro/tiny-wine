set -e

rm -rf *.o *.exe *.so

echo "compiling"
nasm -g -o std_lib.o -l std_lib.lst -f elf64 std_lib.asm
nasm -g -o test.o -l test.lst -f elf64 test.asm

echo "linking shared lib"
ld -shared -o libstdasm.so std_lib.o
# gcc -shared -o libstdasm.so std_lib.o
# gcc -Wall -nostdlib -o test.exe ./libstdasm.so test.o 
echo "linking executable"
gcc -nostartfiles -Wall -o test.exe test.o ./libstdasm.so 

# nasm -o test.bin -f bin test.asm
# ld -o test.exe std_lib.o test.o
# ld -o test.exe test.o ./libstdasm.so 
# cp test.exe test.exe.cpy
# ./test.exe hello_sailor
