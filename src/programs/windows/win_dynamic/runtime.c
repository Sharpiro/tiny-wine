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

        "mov r12, [rsp]\n"
        "lea r13, [rsp + 8]\n"
        "sub rsp, 0x20\n" // Add required stack space
        "sub rsp, 0x08\n" // Add stack alignment
        "call _pei386_runtime_relocator\n"
        "mov ecx, r12d\n"
        "mov rdx, r13\n"
        "call main\n"
        "add rsp, 0x28\n"
        "mov rcx, rax\n"
        "jmp exit\n"
    );
}
