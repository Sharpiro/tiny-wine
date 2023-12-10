void print_shared_lib(void) {
    char *message = "shared lib\n";
    asm("mov rdi, 1");
    asm("mov rsi, %0" : : "r"(message));
    asm("mov rdx, 11");
    asm("mov rax, 1");
    asm("syscall");
}
