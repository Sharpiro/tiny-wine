// #include "win_dynamic_lib_full.h"
// #include "../../../dlls/win_type.h"
// #include <stddef.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <wchar.h>

// int32_t exe_global_var_bss = 0;
// int32_t exe_global_var_data = 42;

// @todo: args don't work

// char *to_char_pointer(wchar_t *data) {
//     size_t length = wcslen(data);
//     char *buffer = malloc(length + 1);
//     size_t i;
//     for (i = 0; i < length; i++) {
//         buffer[i] = (char)data[i];
//     }
//     buffer[i] = 0x00;
//     return buffer;
// }

// extern char *_acmdln;

int main(int argc, char **argv, char **envp) {
    return argc;
    // size_t *argv_ref = (size_t *)0x14000c020;
    // size_t *argc_ref = (size_t *)0x14000c028;
    // printf("argv %p, %zx\n", argv_ref, *argv_ref);
    // printf("argc %p, %zx\n", argc_ref, *argc_ref);
    // printf(
    //     "args: %d, %#zx, %#zx, %s\n", argc, (size_t)argv, (size_t)*argv,
    //     *argv
    // );
    // TEB *teb = NULL;
    // __asm__("mov %0, gs:[0x30]\n" : "=r"(teb));
    // printf("teb: %p, %p\n", teb, teb->Reserved1[0]);
    // printf("_acmdln: %s\n", _acmdln);

    // UNICODE_STRING *image_path_name =
    //     &teb->ProcessEnvironmentBlock->ProcessParameters->ImagePathName;
    // printf(
    //     "teb: %p, %d, %d\n",
    //     teb,
    //     image_path_name->Length,
    //     image_path_name->MaximumLength
    // );
    // char *idk = to_char_pointer((wchar_t *)image_path_name->Buffer);
    // printf("image_path_name: %s\n", idk);

    // printf("%d, %s\n", argc, *argv);
    // printf("look how far we've come\n");
    // printf(
    //     "large params: 1, %x, %x, %x, %x, %x, %x, %x\n", 2, 3, 4, 5, 6, 7, 8
    // );
    // printf("uint32: %x, uint64: %zx\n", 0x12345678, 0x1234567812345678);

    // uint32_t *buffer = malloc(0x1000);
    // buffer[0] = 0xffffffff;
    // printf("malloc: %x\n", buffer[0]);

    // printf(
    //     "stdin: %d, stdout: %d, stderr: %d\n",
    //     fileno(stdin),
    //     fileno(stdout),
    //     fileno(stderr)
    // );

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
    // lib_var_data = 44;
    // printf("lib_var_data: %zd\n", lib_var_data);
    // printf("*get_lib_var_data(): %zd\n", *get_lib_var_data());
}
