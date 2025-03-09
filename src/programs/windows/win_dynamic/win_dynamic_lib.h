#pragma once

#include <stdint.h>

extern uint32_t lib_var_bss;
extern uint32_t lib_var_data;

uint32_t *get_lib_var_bss();

uint32_t *get_lib_var_data();
