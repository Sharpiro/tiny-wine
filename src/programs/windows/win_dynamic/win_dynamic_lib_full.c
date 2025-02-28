#include <stdint.h>

int32_t lib_var_bss = 0;
int32_t lib_var_data = 42;

int32_t get_lib_var_bss() {
    return lib_var_bss;
}
