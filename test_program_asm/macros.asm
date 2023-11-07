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

%macro puts_len 2
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    lea rsi, %1
    mov rdx, %2
    syscall
%endmacro

%macro puts_macro 1
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, %1
    mov r8, -0x01
.seek_char:
    add r8, 1
    mov r9b, [rsi + r8]
    cmp r9b, 0x00
    jne .seek_char
    mov rdx, r8
    syscall

    ; new line
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, new_line
    mov rdx, 1
    syscall
%endmacro

puts:
    mov rsi, rdi
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov r8, -0x01
.seek_char:
    add r8, 1
    mov r9b, [rsi + r8]
    cmp r9b, 0x00
    jne .seek_char
    mov rdx, r8
    syscall

    ; new line
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, new_line
    mov rdx, 1
    syscall
    ret
