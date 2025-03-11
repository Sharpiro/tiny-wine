#pragma once

#include <stdint.h>

extern uint64_t lib_var_bss;
extern uint64_t lib_var_data;

uint64_t *get_lib_var_bss();

uint64_t *get_lib_var_data();

int large(int a, int b, int c, int d, int e, int f, int g, int h);
