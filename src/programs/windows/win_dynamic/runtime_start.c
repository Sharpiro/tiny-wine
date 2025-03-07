#include "../../../dlls/msvcrt.h"
#include <stddef.h>

// @todo: relocations probably

/*
 * Inserted into and called in the `main` function.
 * Replaces manually setting the global 'initialized' symbol used during stdlib
 * TLS setup.
 */
void __main(void) {
}

/*
 * Used in older Clang versions for TLS setup.
 */
void *__p__acmdln() {
    static size_t x = 0;
    return &x;
}

#ifdef NO_STDLIB

/**
 * Required by mingw when relocations are necessary.
 * Mingw expects the startup runtime to call this.
 * In 'nostdlib' and 'stdlib' cases, we'll just have the loader handle
 * relocations.
 */
void _pei386_runtime_relocator(void) {
}

int main();

void mainCRTStartup() {
    int result = main();
    exit(result);
}

#endif
