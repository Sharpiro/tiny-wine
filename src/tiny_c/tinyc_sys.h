#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

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
