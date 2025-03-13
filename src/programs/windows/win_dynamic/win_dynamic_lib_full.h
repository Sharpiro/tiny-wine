#pragma once

#include <stdint.h>

extern uint64_t lib_var_bss;
extern uint64_t lib_var_data;

uint64_t *get_lib_var_bss();

uint64_t *get_lib_var_data();

int large_params(
    int one,
    int two,
    int three,
    int four,
    int five,
    int six,
    int seven,
    int eight
);
