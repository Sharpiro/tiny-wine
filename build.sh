set -e

gcc -Wall -Wextra -g -o linker \
  -masm=intel \
  -Wl,--section-start=.interp=0x900000 \
  -Wno-c2x-extensions \
  -Wno-gnu-empty-initializer \
  -Wno-language-extension-token \
  -fno-stack-protector \
  main.c
