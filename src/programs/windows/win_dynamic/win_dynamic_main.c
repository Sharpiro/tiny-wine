#include "../../../dlls/msvcrt.h"
#include "./win_dynamic_lib.h"

int32_t exe_global_var_bss = 0x00;
int32_t exe_global_var_data = 0x42;

int start_inferior() {
    int32_t num1 = (int32_t)pow(2, 4);
    int32_t num2 = (int32_t)pow(2, 4);
    printf("%d + %d = %d\n", num1, num2, num1 + num2);

    printf("exe_global_var_bss: %x\n", exe_global_var_bss);
    exe_global_var_bss = 1;
    printf("exe_global_var_bss: %x\n", exe_global_var_bss);

    printf("exe_global_var_data: %x\n", exe_global_var_data);
    exe_global_var_data = 0x24;
    printf("exe_global_var_data: %x\n", exe_global_var_data);

    printf("\n");

    printf("*get_lib_var_bss(): %x\n", *get_lib_var_bss());
    printf("lib_var_bss: %x\n", lib_var_bss);
    lib_var_bss += 1;
    printf("lib_var_bss: %x\n", lib_var_bss);
    lib_var_bss = 0x44;
    printf("lib_var_bss: %x\n", lib_var_bss);
    printf("*get_lib_var_bss(): %x\n", *get_lib_var_bss());

    printf("*get_lib_var_data(): %x\n", *get_lib_var_data());
    printf("lib_var_data: %x\n", lib_var_data);
    lib_var_data += 1;
    printf("lib_var_data: %x\n", lib_var_data);
    lib_var_data = 0x44;
    printf("lib_var_data: %x\n", lib_var_data);
    printf("*get_lib_var_data(): %x\n", *get_lib_var_data());

    printf(
        "add_many_msvcrt: %d\n",
        (int32_t)add_many_msvcrt(1, 2, 3, 4, 5, 6, 7, 8)
    );

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

/**
 * Required for accessing mingw dynamic library variables.
 * Seemingly never called.
 */
void _pei386_runtime_relocator(void) {
    exit(255);
}
