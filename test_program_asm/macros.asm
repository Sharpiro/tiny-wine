%macro puts_len 2
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    lea rsi, %1
    mov rdx, %2
    syscall
%endmacro
