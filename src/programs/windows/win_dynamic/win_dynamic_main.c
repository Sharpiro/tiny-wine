#include "../../../dlls/msvcrt.h"
#include "./win_dynamic_lib.h"

int32_t exe_global_var_bss = 0;
int32_t exe_global_var_data = 42;

int start_inferior() {
    printf("pow: %d\n", (int32_t)pow(2, 4));

    /** .bss and .data init */

    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);
    // exe_global_var_bss = 1;
    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);

    // printf("exe_global_var_data: %d\n", exe_global_var_data);
    // exe_global_var_data = 24;
    // printf("exe_global_var_data: %d\n", exe_global_var_data);

    // printf("*get_lib_var_bss(): %d\n", *get_lib_var_bss());
    // printf("lib_var_bss: %d\n", lib_var_bss);
    // lib_var_bss += 1;
    // printf("lib_var_bss: %d\n", lib_var_bss);
    // lib_var_bss = 44;
    // printf("lib_var_bss: %d\n", lib_var_bss);
    // printf("*get_lib_var_bss(): %d\n", *get_lib_var_bss());

    printf("*get_lib_var_data(): %d\n", *get_lib_var_data());
    printf("lib_var_data: %d\n", lib_var_data);
    lib_var_data += 1;
    printf("lib_var_data: %d\n", lib_var_data);
    lib_var_data = 44;
    printf("lib_var_data: %d\n", lib_var_data);
    printf("*get_lib_var_data(): %d\n", *get_lib_var_data());

    // /* Windows to Linux call ,volatile registers, many args */

    // // @todo: won't run on windows b/c this isn't a real msvcrt function

    // size_t rdi, rsi;
    // __asm__(
    //     /**/
    //     "mov rdi, 0x42\n"
    //     "mov rsi, 0x43\n"
    // );
    // int32_t add_many_result = (int32_t)add_many_msvcrt(1, 2, 3, 4, 5, 6, 7,
    // 8);
    // __asm__(
    //     /**/
    //     "mov %0, rdi\n"
    //     "mov %1, rsi\n"
    //     : "=r"(rdi), "=r"(rsi)
    // );
    // printf("add_many_msvcrt: %d\n", add_many_result);
    // printf("add_many_msvcrt rdi, rsi: %x, %x\n", (uint32_t)rdi,
    // (uint32_t)rsi);

    return 0;
}

void mainCRTStartup() {
    int result = start_inferior();
    exit(result);
}

void _start() {
    int result = start_inferior();
    exit(result);
}

// @todo: relocations probably

/**
 * Required for accessing mingw dynamic library variables.
 * Seemingly never called.
 */
void _pei386_runtime_relocator(void) {
    exit(255);
}
