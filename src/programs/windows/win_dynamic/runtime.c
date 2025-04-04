#include <stddef.h>

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
        "mov rcx, 0x00\n"
        "mov rdx, 0x00\n"
        "call _initterm\n"
        "lea rcx, [rsp]\n"
        "lea rdx, [rsp+0x08]\n"
        "lea r8,  [rsp+0x10]\n"
        "call __getmainargs\n"
        "mov rcx, [rsp]\n"
        "mov rdx, [rsp+0x08]\n"
        "mov r8,  [rsp+0x10]\n"
        "call main\n"
        "add rsp, 0x28\n"
        "ret\n"
    );
}
