#pragma once

#include <stdint.h>

extern uint64_t lib_var_bss;
extern uint64_t lib_var_data;

uint64_t *get_lib_var_bss();

uint64_t *get_lib_var_data();
