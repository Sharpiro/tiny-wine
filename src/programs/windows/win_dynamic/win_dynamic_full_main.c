#include "win_dynamic_lib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int32_t exe_global_var_bss = 0;
int32_t exe_global_var_data = 42;

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

    // /** .bss and .data init */

    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);
    // exe_global_var_bss = 1;
    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);

    // printf("exe_global_var_data: %d\n", exe_global_var_data);
    // exe_global_var_data = 24;
    // printf("exe_global_var_data: %d\n", exe_global_var_data);

    // printf("*get_lib_var_bss(): %x\n", *get_lib_var_bss());
    // printf("lib_var_bss: %x\n", lib_var_bss);
    // lib_var_bss += 1;
    // printf("lib_var_bss: %x\n", lib_var_bss);
    // lib_var_bss = 0x44;
    // printf("lib_var_bss: %x\n", lib_var_bss);
    // printf("*get_lib_var_bss(): %x\n", *get_lib_var_bss());

    // printf("*get_lib_var_data(): %x\n", *get_lib_var_data());
    // printf("lib_var_data: %x\n", lib_var_data);
    // lib_var_data += 1;
    // printf("lib_var_data: %x\n", lib_var_data);
    // lib_var_data = 0x44;
    // printf("lib_var_data: %x\n", lib_var_data);
    // printf("*get_lib_var_data(): %x\n", *get_lib_var_data());
}
