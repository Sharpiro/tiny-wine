#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void start_inferior() {
    puts("real libc puts");
}

int mainCRTStartup() {
    start_inferior();
    return 0;
}

void _start() {
    start_inferior();
    exit(0);
}
