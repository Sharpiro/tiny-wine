#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int start_inferior() {
    const char *DATA = "my libc puts";
    int result = puts(DATA);
    result = result < 0 ? result : 0;

    return result;
}

void mainCRTStartup() {
    int result = start_inferior();
    exit(result);
}

void _start() {
    int result = start_inferior();
    exit(result);
}
