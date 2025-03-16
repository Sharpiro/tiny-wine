#include "../../../dlls/msvcrt.h"

/**
 * Required by MinGW when relocations are necessary.
 * MinGW expects the startup runtime to call this.
 */
void _pei386_runtime_relocator(void) {
}

/*
 * Inserted into and called in the `main` function.
 * Replaces setting global 'initialized' symbol during stdlib TLS setup.
 */
void __main(void) {
}

__attribute__((naked)) void mainCRTStartup() {
    __asm__(

        "sub rsp, 0x20\n" // Add stack shadow space
        "sub rsp, 0x08\n" // Add stack alignment
        "call _pei386_runtime_relocator\n"
        "call main\n"
        "add rsp, 0x28\n"
        "ret\n"
    );
}
