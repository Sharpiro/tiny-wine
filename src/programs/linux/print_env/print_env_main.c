#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tiny_c.h>

void _start(void) {
    size_t *frame_pointer = (size_t *)GET_REGISTER("fp");
    size_t argc = frame_pointer[1];
    char **argv = (char **)(frame_pointer + 2);

    tiny_c_printf("%x\n", argc);

    // Print args
    for (size_t i = 0; i < argc; i++) {
        tiny_c_printf("%s\n", argv[i]);
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

    tiny_c_exit(0);
}
