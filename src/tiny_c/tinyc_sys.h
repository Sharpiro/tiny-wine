#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/utsname.h>

struct SysArgs {
    size_t param_one;
    size_t param_two;
    size_t param_three;
    size_t param_four;
    size_t param_five;
    size_t param_six;
    size_t param_seven;
};

size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args);

size_t tinyc_sys_brk(size_t brk);

off_t tinyc_sys_lseek(uint32_t fd, off_t offset, uint32_t whence);

size_t tinyc_sys_uname(struct utsname *uname);

size_t tinyc_sys_arch_prctl(size_t code, size_t address);

size_t tinyc_sys_prctl(
    size_t option, size_t arg2, size_t arg3, size_t arg4, size_t arg5
);
