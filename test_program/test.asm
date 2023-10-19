BITS 64

SYS_WRITE equ 1
SYS_EXIT equ 60
STDOUT equ 1

_start:
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, msg
    mov rdx, msg_len
    syscall

    mov rax, SYS_EXIT
    xor rdi, rdi
    syscall

section .text
    global _start
section .data
    msg db `Hello, Wine\n`
    msg_len equ $ - msg
