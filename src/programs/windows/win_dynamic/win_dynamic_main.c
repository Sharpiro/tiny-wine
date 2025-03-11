#include "../../../dlls/msvcrt.h"
#include "./win_dynamic_lib.h"

int32_t exe_global_var_bss = 0;
int32_t exe_global_var_data = 42;

int main() {
    // printf("pow: %d\n", (int32_t)pow(2, 4));

    // /** .bss and .data init */

    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);
    // exe_global_var_bss = 1;
    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);

    // printf("exe_global_var_data: %d\n", exe_global_var_data);
    // exe_global_var_data = 24;
    // printf("exe_global_var_data: %d\n", exe_global_var_data);

    // printf("*get_lib_var_bss(): %zd\n", *get_lib_var_bss());
    // printf("lib_var_bss: %zd\n", *((uint64_t *)lib_var_bss));
    // *((uint64_t *)lib_var_bss) += 1;
    // printf("lib_var_bss: %zd\n", *((uint64_t *)lib_var_bss));
    // *((uint64_t *)lib_var_bss) = 44;
    // printf("lib_var_bss: %zd\n", *((uint64_t *)lib_var_bss));
    // printf("*get_lib_var_bss(): %zd\n", *get_lib_var_bss());

    // printf("*get_lib_var_data(): %zd\n", *get_lib_var_data());
    // printf("lib_var_data: %zd\n", *((uint64_t *)lib_var_data));
    // *((uint64_t *)lib_var_data) += 1;
    // printf("lib_var_data: %zd\n", *((uint64_t *)lib_var_data));
    // *((uint64_t *)lib_var_data) += 1;
    // printf("lib_var_data: %zd\n", *((uint64_t *)lib_var_data));
    // printf("*get_lib_var_data(): %zd\n", *get_lib_var_data());

    // printf(
    //     "large params: 1, %x, %x, %x, %x, %x, %x, %x\n", 2, 3, 4, 5, 6, 7, 8
    // );
    large(1, 2, 3, 4, 5, 6, 7, 8);
    // printf("uint32: %x, uint64: %zx\n", 0x12345678, 0x1234567812345678);
}
