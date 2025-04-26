#include "../../../dlls/twlibc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern char **environ;

int main(int argc, char *argv[]) {
    printf("%d\n", argc);

    // Print args
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    if (argc <= 3) {
        return 0;
    }

    // Print env vars
    char **envv = environ;
    char *env;
    while ((env = *envv++) != NULL) {
        printf("%s\n", env);
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
        printf("%zd: {%zx, %zx}\n", i, aux.key, aux.value);
    }
}
