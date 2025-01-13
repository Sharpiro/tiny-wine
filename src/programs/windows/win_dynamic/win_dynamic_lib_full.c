#include <stdint.h>

int32_t lib_var = 42;

int lib_add(int x, int y) {
    return lib_var + x + y;
}
