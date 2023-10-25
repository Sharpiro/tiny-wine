set -e

gcc -Wall -g -o linker.exe \
  -masm=intel \
  -Wl,--section-start=.interp=0x900000 \
  main.c
./linker.exe
