%include "std_lib.inc"

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

print_string_array:
    push rdi
    mov r8, rdi
.loop:
    mov rdi, [r8]
    push r8
    call puts
    pop r8
    add r8, 0x08
    cmp qword [r8], 0x00
    jne .loop
    pop rdi
    sub r8, rdi
    add r8, 0x08
    mov rax, r8
    ret

section .data
    new_line db 0x0a
section .text
  global puts
  global print_string_array
