#include <stddef.h>

char **environ = NULL;

int main(int argc, char **argv);

#ifdef ARM32

__attribute__((naked)) void _start(void) {
    __asm__("ldr r0, [sp]\n"
            "add r1, sp, #4\n"
            "bl init\n"
            "mov r7, #1\n"
            "svc #0\n");
}

#endif

__attribute__((used)) static void init(int argc, char **argv) {
    environ = argv + argc + 1;

    main(argc, argv);
}
