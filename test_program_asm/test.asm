BITS 64

SYS_WRITE equ 0x01
SYS_EXIT equ 0x3c
STDOUT equ 0x01

; %idefine rip rel $

_start:
    ; ; print inline
    ; mov rax, 0x6363636363636363
    ; push rax
    ; mov rax, SYS_WRITE
    ; mov rdi, STDOUT
    ; mov rsi, rsp
    ; mov rdx, 8
    ; syscall

    ; ; jmp test_func
    ; mov rax, test_func
    ; push test_func ; 0x40100e
    ; push 0xff
    ; jmp [rsp+8]
    ; mov rax, 0xaa
    ; jmp -0x6
    ; jmp rip
    ; jmp [rel $]
    ; mov eax, [rel $]
    ; mov rax, 0xbb

    ; ; print msg
    ; mov rax, SYS_WRITE
    ; mov rdi, STDOUT
    ; mov rsi, msg
    ; mov rdx, msg_len
    ; syscall

    ; print msg_two
    mov rax, SYS_WRITE
    mov rdi, STDOUT
    mov rsi, msg_two
    mov rdx, msg_two_len
    syscall

  ; exit
  mov rbx, 0xaa
  mov rax, SYS_EXIT
  mov rdi, 0
  syscall

test_func:
  mov rbx, 0xaa
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
