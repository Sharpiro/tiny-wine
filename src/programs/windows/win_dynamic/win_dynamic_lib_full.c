#include <stdbool.h>
#include <stdint.h>

uint64_t lib_var_bss = 0;
uint64_t lib_var_data = 42;

uint64_t *get_lib_var_bss() {
    return &lib_var_bss;
}

uint64_t *get_lib_var_data() {
    return &lib_var_data;
}
