#include <stddef.h>
#include <stdio.h>

int mainCRTStartup() {
    puts("real libc puts");

    return 0x45;
}
