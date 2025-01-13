#include <stdbool.h>
#include <stdint.h>

bool DllMain(
    [[maybe_unused]] void *hinstDLL,
    [[maybe_unused]] unsigned long fdwReason,
    [[maybe_unused]] void *lpReserved
) {
    return true;
}

// int32_t lib_var = 42;

int lib_add(int x, int y) {
    return x + y;
}
