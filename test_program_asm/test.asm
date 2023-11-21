BITS 64

%include "std_lib.inc"
%include "macros.asm"

; %idefine rip rel $

_start:
    ; print arg count
    mov rdi, [rsp]
    call print_number_32
    puts_line

    ; print args
    lea rdi, [rsp + 8]
    push rdi
    call print_string_array
    pop rdi

    ; print env vars
    add rdi, rax
    push rdi
    call print_string_array
    pop rdi

    ; print auxiliary vector
    add rdi, rax
    push rdi
    call print_auxiliary_vector

    mov rdi, 0
    call libc_exit

section .text
    global _start
section .data
    new_line db 0x0a
    msg db `aaaaaaaaaaaaaaa\n`
    msg_len equ $ - msg
