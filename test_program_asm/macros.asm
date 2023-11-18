%macro puts_len 2
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    lea rsi, %1
    mov rdx, %2
    syscall
%endmacro

%macro puts_line 0
    puts_len new_line, 1
%endmacro
