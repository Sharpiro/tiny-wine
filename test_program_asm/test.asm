BITS 64

%include "std_lib.inc"
%include "macros.asm"

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

    ; ; print args
    ; lea rdi, [rsp + 8]
    ; push rdi
    ; call print_string_array
    ; pop rdi
    ; ; print env vars
    ; add rdi, rax
    ; push rdi
    ; call print_string_array
    ; pop rdi
    ; print anciliary vector
    ; add rdi, rax
    ; mov rdi, 0x09
    ; call print_small_number 
    mov rdi, 0x000000ab
    call print_number
    ; mov rdi, 0x10
    ; call print_number
; .loop:
;     mov rdi, [r13]
;     call puts
;     add r13, 8
;     cmp qword [r13], 0
;     jne .loop

;     add r13, 8
; .loop2:
;     mov rdi, [r13]
;     call puts
;     add r13, 8
;     cmp qword [r13], 0
;     jne .loop2


  ; exit
  mov rbx, 0xaa
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
