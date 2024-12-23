// #include "ntdll.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

size_t sys_write(int32_t file_handle, const char *data, size_t size);

void start_inferior() {
    // puts("real libc puts");
    sys_write(1, "real libc puts\n", 15);
}

int mainCRTStartup() {
    start_inferior();
    return 0;
}

void _start() {
    start_inferior();
    exit(0);
}
