#pragma once

#include <stddef.h>
#include <stdint.h>

struct SysArgs {
    size_t param_one;
    size_t param_two;
    size_t param_three;
    size_t param_four;
    size_t param_five;
    size_t param_six;
    size_t param_seven;
};

size_t tinyc_sys_brk(size_t brk);
