#include "../../../dlls/msvcrt.h"
#include <stdint.h>

// int32_t exe_global_var_bss = 0;
// int32_t exe_global_var_data = 42;

int start_inferior() {
    int32_t local_errno = errnox;
    printf("%x\n", local_errno);
    return 0;

    // volatile int *temp_err = &errnox;
    // errno = 99;
    // int32_t num1 = (int32_t)pow(2, 4);
    // int32_t num2 = (int32_t)pow(2, 4);
    // printf("%d + %d = %d\n", num1, num2, num1 + num2);

    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);
    // exe_global_var_bss = 1;
    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);

    // printf("exe_global_var_data: %d\n", exe_global_var_data);
    // exe_global_var_data = 24;
    // printf("exe_global_var_data: %d\n", exe_global_var_data);

    // printf("%d\n", 1);

    // errno += 1;
    // errno = 2;
    // return *temp_err;
    // return 0;
}

void mainCRTStartup() {
    int result = start_inferior();
    exit(result);
}

void _start() {
    int result = start_inferior();
    exit(result);
}

/**
 * Required for accessing mingw dynamic library variables.
 * Seemingly never called.
 */
void _pei386_runtime_relocator(void) {
    exit(255);
}
