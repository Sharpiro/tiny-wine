#pragma once

#include <macros.h>
#include <stdint.h>

EXPORTABLE extern uint64_t lib_var_bss;
EXPORTABLE extern uint64_t lib_var_data;

EXPORTABLE uint64_t *get_lib_var_bss();

EXPORTABLE uint64_t *get_lib_var_data();

EXPORTABLE int large_params(
    int one,
    int two,
    int three,
    int four,
    int five,
    int six,
    int seven,
    int eight
);
