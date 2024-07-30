#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tiny_c.h>

__attribute__((naked)) void _start(void) {
    __asm__("ldr r0, [sp]\n"
            // "add r1, sp, #4\n"
            // @todo: no point in not using const w/ this asm
            "add r1, sp, %[p1]\n"
            "bl main\n"
            "mov r7, #1\n"
            "svc #0\n" ::[p1] "r"(sizeof(size_t))
            :);
}

int main(int argc, char *argv[]) {
    return -1;
    tiny_c_printf("%x\n", argc);

    // Print args
    for (int i = 0; i < argc; i++) {
        tiny_c_printf("%s\n", argv[i]);
    }

    if (argc <= 3) {
        return 0;
    }

    // Print env vars
    char **envv = argv + argc + 1;
    char *env;
    while ((env = *envv++) != NULL) {
        tiny_c_printf("%s\n", env);
    }

    // Print auxiliary vector
    struct AuxEntry {
        size_t key;
        size_t value;
    };
    struct AuxEntry *auxv = (struct AuxEntry *)envv;
    for (size_t i = 0; true; i += 1) {
        struct AuxEntry aux = auxv[i];
        if (aux.key == 0 && aux.value == 0) {
            break;
        }
        tiny_c_printf("{%x, %x}\n", aux.key, aux.value);
    }
}
