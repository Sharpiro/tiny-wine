BITS 64

%include "macros.asm"

SYS_WRITE equ 0x01
SYS_EXIT equ 0x3c
STDOUT equ 0x01

; %idefine rip rel $

extern add_num

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
    ; call print_message
    ; mov rdi, 0x08
    ; mov rsi, 0x04
    ; call add_num

    ; ; print msg_two
    ; mov rax, SYS_WRITE
    ; mov rdi, STDOUT
    ; mov rsi, msg_two
    ; mov rdx, msg_two_len
    ; syscall

    ; print arg count
    ; mov r10, [rsp]
    ; print_small_number r10

    ; print args
    ; mov r10, [rsp + 16]
    ; puts_func r10
    mov r12, 0
    mov r13, [rsp]
.temp_label:
    mov rdi, [rsp + ((r12 + 1) * 8)]
    call puts
    add r12, 1
    cmp r12, r13
    jl .temp_label
    ; mov rdi, [rsp + 16]
    ; call puts


  ; exit
  mov rbx, 0xaa
  ; mov rdi, rax
  mov rax, SYS_EXIT
  mov rdi, 0
  syscall

print_message:
  mov rax, SYS_WRITE
  mov rdi, STDOUT
  mov rsi, msg
  mov rdx, msg_len
  syscall
  ret

section .text
    global _start
section .data
    msg db `aaaaaaaaaaaaaaa\n`
    msg_len equ $ - msg
    msg_two db `bbbbbbbbbbbbbbb\n`
    msg_two_len equ $ - msg_two
    new_line db 0x0a
