#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void start_inferior() {
    const char *DATA = "my libc puts";
    puts(DATA);
}

void mainCRTStartup() {
    start_inferior();
    exit(0);
}

void _start() {
    start_inferior();
    exit(0);
}
