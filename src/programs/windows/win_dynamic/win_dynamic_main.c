#include "../../../dlls/msvcrt.h"
// #include "./win_dynamic_lib.h"

// int32_t exe_global_var_bss = 0;
// int32_t exe_global_var_data = 42;

// @todo: duped
char *to_char_pointer(wchar_t *data) {
    size_t length = wcslen(data);
    char *buffer = malloc(length + 1);
    size_t i;
    for (i = 0; i < length; i++) {
        buffer[i] = (char)data[i];
    }
    buffer[i] = 0x00;
    return buffer;
}

// @todo: args don't work
int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    // printf("args: %d, %zx: ", argc, (size_t)argv);
    // for (int i = 0; i < argc; i++) {
    //     printf("'%s', ", argv[i]);
    // }
    // printf("\n");

    printf("_acmdln: %p, '%s'\n", _acmdln, _acmdln);

    TEB *teb = NULL;
    __asm__("mov %0, gs:[0x30]\n" : "=r"(teb));
    UNICODE_STRING *command_line =
        &teb->ProcessEnvironmentBlock->ProcessParameters->CommandLine;
    printf(
        "teb: %p, %d, %d\n",
        teb,
        command_line->Length,
        command_line->MaximumLength
    );
    printf(
        "command_line: %p, '%s'\n",
        command_line->Buffer,
        to_char_pointer((wchar_t *)command_line->Buffer)
    );

    // int main() {
    // TEB *teb = NULL;
    // size_t *teb = NULL;
    // __asm__("move %0, gs:[0x30]\n" : "=r"(teb));
    // printf("teb: %p\n", teb);
    // printf("%d, %s\n", argc, *argv);
    // printf("uint32: %x, uint64: %zx\n", 0x12345678, 0x1234567812345678);
    // printf("pow: %d\n", (int32_t)pow(2, 4));

    // /** .bss and .data init */

    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);
    // exe_global_var_bss = 1;
    // printf("exe_global_var_bss: %d\n", exe_global_var_bss);

    // printf("exe_global_var_data: %d\n", exe_global_var_data);
    // exe_global_var_data = 24;
    // printf("exe_global_var_data: %d\n", exe_global_var_data);

    // printf("*get_lib_var_bss(): %zd\n", *get_lib_var_bss());
    // printf("lib_var_bss: %zd\n", lib_var_bss);
    // lib_var_bss += 1;
    // printf("lib_var_bss: %zd\n", lib_var_bss);
    // lib_var_bss = 44;
    // printf("lib_var_bss: %zd\n", lib_var_bss);
    // printf("*get_lib_var_bss(): %zd\n", *get_lib_var_bss());

    // printf("*get_lib_var_data(): %zd\n", *get_lib_var_data());
    // printf("lib_var_data: %zd\n", lib_var_data);
    // lib_var_data += 1;
    // printf("lib_var_data: %zd\n", lib_var_data);
    // lib_var_data += 1;
    // printf("lib_var_data: %zd\n", lib_var_data);
    // printf("*get_lib_var_data(): %zd\n", *get_lib_var_data());

    // /* Preserved register state */

    // DEBUG_SET_MACHINE_STATE();
    // size_t rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14, r15, rbp;
    // GET_PRESERVED_REGISTERS();
    // size_t rbp_saved = rbp;
    // large_params(1, 2, 3, 4, 5, 6, 7, 8);
    // GET_PRESERVED_REGISTERS();
    // printf(
    //     "preserved registers: %zx, %zx, %zx, %zx, %zx, %zx, %zx, %x\n",
    //     rbx,
    //     rdi,
    //     rsi,
    //     r12,
    //     r13,
    //     r14,
    //     r15,
    //     rbp == rbp_saved ? 8 : 0
    // );
}
