#include "../../../dlls/msvcrt.h"

// @todo: relocations probably

/**
 * Required by MinGW when relocations are necessary.
 * MinGW expects the startup runtime to call this.
 */
void _pei386_runtime_relocator(void) {
    // fprintf(stderr, "_pei386_runtime_relocator\n");
    // exit(99);
}

/*
 * Inserted into and called in the `main` function.
 * Replaces setting global 'initialized' symbol during stdlib TLS setup.
 */
void __main(void) {
}

int main();

void mainCRTStartup() {
    _pei386_runtime_relocator();
    int result = main();
    exit(result);
}
