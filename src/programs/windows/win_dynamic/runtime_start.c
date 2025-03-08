#include "../../../dlls/msvcrt.h"
#include <stddef.h>

// @todo: relocations probably

// @todo: overriding these functions through the linker is less flexible
//        than at runtime through memory poking b/c now you are requiring the
//        program to be built with these rather than just being built with a
//        similar compiler. Another option is to tell the loader to override
//        these functions at load time which would let us avoid awkward memory
//        poking, but still give us the flexibility.

/*
 * Inserted into and called in the `main` function.
 * Replaces setting global 'initialized' symbol during stdlib TLS setup.
 */
void __main(void) {
}

/*
 * Used in older Clang versions for TLS setup.
 * Replaces setting global '.refptr.__imp__acmdln' symbol during stdlib TLS
 * setup.
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
