%include "std_lib.inc"
%include "macros.asm"

DEFAULT REL

puts:
    mov rsi, rdi
    mov r8, -0x01
.seek_char:
    add r8, 1
    mov r9b, [rsi + r8]
    cmp r9b, 0x00
    jne .seek_char
    mov rdx, r8
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    syscall

    puts_line
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

print_number_32:
    mov rbp, rsp
    mov r8d, edi
    mov r12, 0x00
    mov r13, 0x10000000
    mov rsi, 0x00
.calc_byte:
    shr rsi, 0x08
    mov rax, r8
    mov rdx, 0x00
    mov rbx, r13
    div rbx
    mov r14, rax
    mov r9, rax
    cmp r9b, 0x0a
    mov r10, 0x30
    mov r11, 0x57
    cmovge r10, r11
    add r9, r10
    shl r9, 0x38
    or rsi, r9

    ; while
    add r12, 1
    mov rax, r14
    mul r13
    sub r8, rax
    shr r13, 0x04
    cmp r12, 0x08
    jl .calc_byte

    push rsi
    puts_len [rsp], 0x08
    mov rsp, rbp
    ret

print_auxiliary_vector:
    mov r8, rdi
.loop:
    mov rdi, [r8]
    push r8
    call print_number_32 
    puts_len space, 1
    pop r8
    mov rdi, [r8+8]
    push r8
    call print_number_32 
    pop r8
    add r8, 16
    puts_line
    mov r9, [r8]
    cmp r9, 0x00
    jne .loop
    ret

libc_exit:
    mov rax, SYS_EXIT
    syscall
    ret

section .data
    new_line db 0x0a
    space db " "
section .text
  global puts
  global print_string_array
  global libc_exit
