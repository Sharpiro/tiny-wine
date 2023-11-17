set -e

gcc -Wall -g -o linker.exe \
  -masm=intel \
  -Wl,--section-start=.interp=0x900000 \
  -fno-stack-protector \
  -Wno-c2x-extensions \
  main.c
# ./linker.exe
