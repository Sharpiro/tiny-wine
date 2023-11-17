%macro puts_len 2
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    lea rsi, %1
    mov rdx, %2
    syscall
%endmacro

%macro puts_len_reg 2
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, %1
    mov rdx, %2
    syscall
%endmacro

%macro puts_line 0
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, new_line
    mov rdx, 1
    syscall
%endmacro

%macro exit 1
    mov rax, SYS_EXIT
    mov rdi, %1
    syscall
%endmacro
