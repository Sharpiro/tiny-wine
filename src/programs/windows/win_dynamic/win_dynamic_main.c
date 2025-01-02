#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

size_t add_many_msvcrt(
    [[maybe_unused]] size_t one,
    [[maybe_unused]] size_t two,
    [[maybe_unused]] size_t three,
    [[maybe_unused]] size_t four,
    [[maybe_unused]] size_t five,
    [[maybe_unused]] size_t six,
    [[maybe_unused]] size_t seven,
    [[maybe_unused]] size_t eight
);

int start_inferior() {
    // const char *DATA = "my libc puts";
    // int result = puts(DATA);
    size_t result = add_many_msvcrt(1, 2, 3, 4, 5, 6, 7, 8);
    // result = result < 0 ? result : 0;

    return (int)result;
}

void mainCRTStartup() {
    int result = start_inferior();
    exit(result);
}

void _start() {
    int result = start_inferior();
    exit(result);
}
