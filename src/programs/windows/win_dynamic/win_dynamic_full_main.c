// #include "../../../dlls/msvcrt.h"
// #include "./win_dynamic_lib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("look how far we've come\n");

    uint32_t *buffer = malloc(0x1000);
    buffer[0] = 0xffffffff;
    printf("malloc: %x\n", buffer[0]);

    printf(
        "stdin: %d, stdout: %d, stderr: %d\n",
        fileno(stdin),
        fileno(stdout),
        fileno(stderr)
    );
}
