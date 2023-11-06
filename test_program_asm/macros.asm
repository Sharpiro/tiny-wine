%macro print_small_number 1
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov r8, 0x0a00
    mov r9, 0x30
    add r9, %1
    or r8, r9
    push r8
    lea rsi, [rsp]
    mov rdx, 2
    syscall
%endmacro

%macro puts 2
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    lea rsi, %1
    mov rdx, %2
    syscall
%endmacro
