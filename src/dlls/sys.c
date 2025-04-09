#include "sys.h"
#include <stddef.h>

// size_t sys_call(size_t sys_no, struct SysArgs *sys_args) {
//     size_t result = 0;

//     __asm__("mov rdi, %0" : : "r"(sys_args->param_one));
//     __asm__("mov rsi, %0" : : "r"(sys_args->param_two));
//     __asm__("mov rdx, %0" : : "r"(sys_args->param_three));
//     __asm__("mov rcx, %0" : : "r"(sys_args->param_four));
//     __asm__("mov r8, %0" : : "r"(sys_args->param_five));
//     __asm__("mov r9, %0" : : "r"(sys_args->param_six));
//     __asm__("mov r10, %0" : : "r"(sys_args->param_seven));
//     __asm__("mov rax, %0" : : "r"(sys_no));
//     __asm__("syscall" : "=r"(result) : :);

//     return result;
// }
