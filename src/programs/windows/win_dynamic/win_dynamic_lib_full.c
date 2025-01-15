#include <stdint.h>

int32_t lib_var_data = 42;

int lib_add(int x, int y) {
    return lib_var_data + x + y;
}
