#include <dlls/twlibc.h>

EXPORTABLE extern uint64_t lib_var_bss;
EXPORTABLE extern uint64_t lib_var_data;

EXPORTABLE uint64_t *get_lib_var_bss();

EXPORTABLE uint64_t *get_lib_var_data();

EXPORTABLE int large_params_dll(
    int one,
    int two,
    int three,
    int four,
    int five,
    int six,
    int seven,
    int eight
);

int32_t ntdll_large_params_msvcrt(
    int32_t one,
    int32_t two,
    int32_t three,
    int32_t four,
    int32_t five,
    int32_t six,
    int32_t seven,
    int32_t eight
);

int32_t exe_global_var_bss = 0;
int32_t exe_global_var_data = 42;

// @todo: add 'WINE' env var to have better testing and compatibility
// @todo: unify w/ 'full' version?

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
    large_params_dll(1, 2, 3, 4, 5, 6, 7, 8);
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

    int32_t large_params_swap =
        ntdll_large_params_msvcrt(1, 2, 3, 4, 5, 6, 7, 8);
    printf("large_params_swap %d\n", large_params_swap);
}
