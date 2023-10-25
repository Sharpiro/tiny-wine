set -e

gcc -Wall -g -o main.exe \
  -masm=intel \
  -Wl,--section-start=.interp=0x900000 \
  main.c
./main.exe
