#include <stdbool.h>
#include <stdint.h>

void print_shared_lib(void);

void print_local(void) {
    char *message = "local\n";
    asm("mov rdi, 1");
    asm("mov rsi, %0" : : "r"(message));
    asm("mov rdx, 6");
    asm("mov rax, 1");
    asm("syscall");
}

int main_inferior(void) {
    print_local();
    print_shared_lib();
    print_shared_lib();

    return 0;
}

void _start(void) {
    uint64_t result = main_inferior();
    // uint64_t result = 0;
    // asm("mov rax, rsp" : "=r"(result));
    // uint64_t *arg_count = (uint64_t *)(result + 8);
    // asm("mov rdi, %0" ::"r"(*arg_count));

    asm("mov rdi, %0" : : "r"(result));
    asm("mov rax, 0x3c");
    asm("syscall");
}
