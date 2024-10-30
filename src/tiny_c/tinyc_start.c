#include <stddef.h>

char **environ = NULL;

int main(int argc, char **argv);

#ifdef ARM32

__attribute__((naked)) void _start(void) {
    __asm__("ldr r0, [sp]\n"
            "add r1, sp, #4\n"
            "bl tinyc_init\n"
            "mov r7, #1\n"
            "svc #0\n");
}

#endif

#ifdef AMD64

__attribute__((naked)) void _start(void) {
    __asm__("mov rdi, [rsp]\n"
            "lea rsi, [rsp + 8]\n"
            "call tinyc_init\n"
            "mov rdi, rax\n"
            "mov rax, 0x3c\n"
            "syscall\n");
}

#endif

__attribute__((used)) static int tinyc_init(int argc, char **argv) {
    environ = argv + argc + 1;

    return main(argc, argv);
}
