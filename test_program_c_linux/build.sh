set -e

  # -Tbss 0x500000 \
  # -Tdata 0x600000 \
  # -static \
  # -Tplt 0x800000 \
  # -T linker.ld \
  # -nostartfiles \
# gcc -std=gnu99 -Wall -Wl,--section-start=.interp=0x800000 test_program.c -o Lala
  # -Ttext 0x700000 \
gcc -Wall -g -o test.exe \
  -Wl,--section-start=.interp=0x900000 \
   test_program.c
# gcc -static test.o -o test.exe

# ld -lc -o test.exe -I /lib/ld-linux-x86-64.so.2 test.o
objdump -D -M intel test.exe > test.dump
./test.exe
