%include "std_lib.inc"
%include "macros.asm"

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

print_small_number:
    mov r8, rdi
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    or r8, 0x0a00
    add r8, 0x30
    push r8
    lea rsi, [rsp]
    mov rdx, 2
    syscall
    pop r8
    retn

print_number:
    mov r8d, edi
    mov r12, 0
    mov r13, 0x10000000
.calc_byte:
    mov rax, r8
    mov rdx, 0
    mov rbx, r13
    div rbx
    mov r14, rax
    mov r9, rax
    cmp r9b, 0x0a
    mov r10, 0x30
    mov r11, 0x57
    cmovge r10, r11
    add r9, r10
    push r9
    puts_len rsp, 1
    pop r9

    ; while
    add r12, 1
    mov rax, r14
    mul r13
    sub r8, rax
    shr r13, 0x04
    cmp r12, 0x08
    jl .calc_byte

    ret

print_auxiliary_vector:
    mov r8, rdi
.aux_loop:
    mov rdi, [r8]
    push r8
    call print_number 
    puts_len space, 1
    pop r8
    mov rdi, [r8+8]
    push r8
    call print_number 
    pop r8
    add r8, 16
    puts_len new_line, 1
    mov r9, [r8]
    cmp r9, 0x00
    jne .aux_loop
    ret

section .data
    new_line db 0x0a
    space db " "
section .text
  global puts
  global print_string_array
