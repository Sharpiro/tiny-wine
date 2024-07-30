#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tiny_c.h>

ARM32_START_FUNCTION

int main(int argc, char *argv[]) {
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
