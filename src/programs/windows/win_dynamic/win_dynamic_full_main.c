// #include "win_dynamic_lib_full.h"
#include "../../../dlls/win_type.h"
// #include <stddef.h>
// #include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// int32_t exe_global_var_bss = 0;
// int32_t exe_global_var_data = 42;

// @todo: args don't work

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

extern char *_acmdln;

int main(int argc, char **argv, [[maybe_unused]] char **envp) {
    // return argc;
    // size_t *argv_ref = (size_t *)0x14000c020;
    // size_t *argc_ref = (size_t *)0x14000c028;
    // printf("argv %p, %zx\n", argv_ref, *argv_ref);
    // printf("argc %p, %zx\n", argc_ref, *argc_ref);

    printf("stack: %#zx\n", (size_t)&argc);
    printf("heap: %#zx\n", (size_t)malloc(0x10));
    printf(
        "args: %d, %#zx, %#zx, '%s'\n", argc, (size_t)argv, (size_t)*argv, *argv
    );

    // for (int i = 0; i < argc; i++) {
    //     printf("arg: %d/%d, '%s'\n", i + 1, argc, argv[i]);
    // }

    // _acmdln = "idk bro";
    *_acmdln = '*';
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
    command_line->Buffer[0] = '$';
    printf(
        "image_path_name: %p, '%s'\n",
        command_line->Buffer,
        to_char_pointer((wchar_t *)command_line->Buffer)
    );

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
