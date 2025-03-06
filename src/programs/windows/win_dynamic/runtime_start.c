#include "../../../dlls/msvcrt.h"
#include <stddef.h>

// @todo: relocations probably

/**
 * Required by mingw when relocations are necessary.
 * Mingw expects the startup runtime to call this.
 * In 'nostdlib' and 'stdlib' cases, we'll just have the loader handle
 * relocations.
 */
void _pei386_runtime_relocator(void) {
}

/*
 * Inserted into and called in the `main` function
 */
void __main(void) {
}

#ifdef NO_STDLIB

int main();

void mainCRTStartup() {
    int result = main();
    exit(result);
}

#endif
