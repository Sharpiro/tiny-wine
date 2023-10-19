set -e
gcc -Wall -o main.exe \
  -g \
  -masm=intel \
  main.c
./main.exe
