#include "../../../dlls/msvcrt.h"
#include "./win_dynamic_lib.h"

int32_t exe_global_var_bss = 0;
int32_t exe_global_var_data = 42;

int main(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        if (i + 1 == argc) {
            printf("'%s'", argv[i]);
        } else {
            printf("'%s', ", argv[i]);
        }
    }
    printf("\n");

    printf(
        "large params: 1, %x, %x, %x, %x, %x, %x, %x\n", 2, 3, 4, 5, 6, 7, 8
    );
    printf("uint32: %x, uint64: %zx\n", 0x12345678, 0x1234567812345678);
    printf("pow: %d\n", (int32_t)pow(2, 4));

    uint32_t *buffer = malloc(0x1000);
    buffer[0] = 0xabcdef01;
    printf("malloc: %x\n", buffer[0]);

    printf(
        "stdin: %d, stdout: %d, stderr: %d\n",
        fileno(stdin),
        fileno(stdout),
        fileno(stderr)
    );

    /** .bss and .data init */

    printf("exe_global_var_bss: %d\n", exe_global_var_bss);
    exe_global_var_bss = 1;
    printf("exe_global_var_bss: %d\n", exe_global_var_bss);

    printf("exe_global_var_data: %d\n", exe_global_var_data);
    exe_global_var_data = 24;
    printf("exe_global_var_data: %d\n", exe_global_var_data);

    printf("*get_lib_var_bss(): %zd\n", *get_lib_var_bss());
    printf("lib_var_bss: %zd\n", lib_var_bss);
    lib_var_bss += 1;
    printf("lib_var_bss: %zd\n", lib_var_bss);
    lib_var_bss = 44;
    printf("lib_var_bss: %zd\n", lib_var_bss);
    printf("*get_lib_var_bss(): %zd\n", *get_lib_var_bss());

    printf("*get_lib_var_data(): %zd\n", *get_lib_var_data());
    printf("lib_var_data: %zd\n", lib_var_data);
    lib_var_data += 1;
    printf("lib_var_data: %zd\n", lib_var_data);
    lib_var_data = 44;
    printf("lib_var_data: %zd\n", lib_var_data);
    printf("*get_lib_var_data(): %zd\n", *get_lib_var_data());

    /* Preserved register state */

    DEBUG_SET_MACHINE_STATE();
    size_t rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14, r15, rbp;
    double xmm0, xmm1;
    GET_REGISTERS_SNAPSHOT();
    size_t rbp_saved = rbp;
    large_params(1, 2, 3, 4, 5, 6, 7, 8);
    GET_REGISTERS_SNAPSHOT();
    printf(
        "preserved registers: %zx, %zx, %zx, %zx, %zx, %zx, %zx, %x\n",
        rbx,
        rdi,
        rsi,
        r12,
        r13,
        r14,
        r15,
        rbp == rbp_saved ? 8 : 0
    );
}
