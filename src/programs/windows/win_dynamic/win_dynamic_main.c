#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int mainCRTStartup() {
    puts("real libc puts");

    return 0x45;
}

void _start() {
    puts("real libc puts");
    exit(0);
}
