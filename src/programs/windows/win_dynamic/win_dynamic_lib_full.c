#include "./win_dynamic_lib.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t lib_var_bss = 0;
uint32_t lib_var_data = 42;

uint32_t *get_lib_var_bss() {
    return &lib_var_bss;
}

uint32_t *get_lib_var_data() {
    return &lib_var_data;
}
