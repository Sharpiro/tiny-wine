BITS 64

SYS_WRITE equ 0x01
SYS_EXIT equ 0x3c
STDOUT equ 0x01

_start:
    ; print inline
    ; mov rax, 0x6363636363636363
    ; push rax
    ; mov rax, SYS_WRITE
    ; mov rdi, STDOUT
    ; ; dereferences?
    ; ; mov rsi, [rsp-4]
    ; mov rsi, rsp
    ; mov rdx, 8
    ; syscall

    ; print msg
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, msg
    mov rdx, msg_len
    syscall

    ; ; print msg_two
    ; mov rax, SYS_WRITE
    ; mov rdi, STDOUT
    ; mov rsi, msg_two
    ; mov rdx, msg_two_len
    ; syscall

    mov rax, SYS_EXIT
    mov rdi, 0
    syscall

section .text
    global _start
section .data
    msg db `aaaaaaaaaaaaaaa\0`
    msg_len equ $ - msg
    msg_two db `bbbbbbbbbbbbbbb\0`
    msg_two_len equ $ - msg_two
