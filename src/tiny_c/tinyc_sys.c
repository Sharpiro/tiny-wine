#include "tinyc_sys.h"
#include <sys/syscall.h>

#ifdef AMD64

#define MMAP SYS_mmap

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

#endif

#ifdef ARM32

#define MMAP SYS_mmap2
// #define MMAP 0xc0

// @todo: don't know how to clobber 7 registers
size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    __asm__("mov r0, %[p1]\n"
            "mov r1, %[p2]\n"
            "mov r2, %[p3]\n"
            "mov r3, %[p4]\n"
            "mov r4, %[p5]\n"
            "mov r5, %[p6]\n"
            "mov r6, %[p7]\n"
            "mov r7, %[sysno]\n"
            "svc #0\n"
            "mov %[res], r0\n"
            : [res] "=r"(result)
            : [p1] "r"(sys_args->param_one),
              [p2] "r"(sys_args->param_two),
              [p3] "r"(sys_args->param_three),
              [p4] "r"(sys_args->param_four),
              [p5] "r"(sys_args->param_five),
              [p6] "r"(sys_args->param_six),
              [p7] "r"(sys_args->param_seven),
              [sysno] "r"(sys_no)
            : "r0", "r1", "r2", "r3", "r4");

    return result;
}

#endif

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
