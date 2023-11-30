// #include <stdio.h>
#include <stdint.h>

// int main(void) {
//     // printf("aaaaaaaa");
//     return 99;
// }

void _start(void) {
    uint64_t result = 2;
    // asm volatile("mov rdi, %0" : : "r"(result) : "rax");
    asm("mov rdi, %0" : : "r"(result));
    asm("mov rax, 0x3c");
    asm("syscall");
}
