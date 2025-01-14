#include "../../../dlls/msvcrt.h"
#include <stdbool.h>
#include <stdint.h>

int32_t lib_var = 42;

/**
 * Called on load, unload, etc.
 */
bool DllMain(
    [[maybe_unused]] void *hinstDLL,
    [[maybe_unused]] unsigned long fdwReason,
    [[maybe_unused]] void *lpReserved
) {
    return true;
}

int lib_add(int x, int y) {
    return x + y;
}
