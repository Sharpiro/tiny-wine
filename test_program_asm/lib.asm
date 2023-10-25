BITS 64

add_num:
  mov rax, 0x00,
  add rax, rdi
  add rax, rsi
  ret

section .text
  global add_num
