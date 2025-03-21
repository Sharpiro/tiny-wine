#include "tinyc_sys.h"
#include <asm/prctl.h>
#include <sys/syscall.h>

size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    __asm__("mov rdi, %0" : : "r"(sys_args->param_one));
    __asm__("mov rsi, %0" : : "r"(sys_args->param_two));
    __asm__("mov rdx, %0" : : "r"(sys_args->param_three));
    __asm__("mov rcx, %0" : : "r"(sys_args->param_four));
    __asm__("mov r8, %0" : : "r"(sys_args->param_five));
    __asm__("mov r9, %0" : : "r"(sys_args->param_six));
    __asm__("mov r10, %0" : : "r"(sys_args->param_seven));
    __asm__("mov rax, %0" : : "r"(sys_no));
    __asm__("syscall" : "=r"(result) : :);

    return result;
}

size_t tinyc_sys_brk(size_t brk) {
    struct SysArgs args = {
        .param_one = brk,
    };
    size_t result = tiny_c_syscall(SYS_brk, &args);
    return result;
}

off_t tinyc_sys_lseek(uint32_t fd, off_t offset, uint32_t whence) {
    struct SysArgs args = {
        .param_one = fd,
        .param_two = (size_t)offset,
        .param_three = whence,
    };
    size_t result = tiny_c_syscall(SYS_lseek, &args);
    off_t result_offset = (off_t)result;
    return result_offset;
}

size_t tinyc_sys_uname(struct utsname *uname) {
    struct SysArgs args = {
        .param_one = (size_t)uname,
    };
    size_t result = tiny_c_syscall(SYS_uname, &args);
    return result;
}

size_t tinyc_sys_arch_prctl(size_t code, size_t address) {
    struct SysArgs args = {
        .param_one = code,
        .param_two = address,
    };
    size_t result = tiny_c_syscall(SYS_arch_prctl, &args);
    return result;
}

size_t tinyc_sys_prctl(
    size_t option, size_t arg2, size_t arg3, size_t arg4, size_t arg5
) {
    struct SysArgs args = {
        .param_one = option,
        .param_two = arg2,
        .param_three = arg3,
        .param_four = arg4,
        .param_five = arg5,
    };
    size_t result = tiny_c_syscall(SYS_prctl, &args);
    return result;
}
