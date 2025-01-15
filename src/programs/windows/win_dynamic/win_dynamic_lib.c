#include "../../../dlls/msvcrt.h"
#include <stdbool.h>
#include <stdint.h>

int32_t lib_var_bss = 0;

int32_t lib_var_data = 0x42;

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

int32_t *get_lib_var_bss() {
    return &lib_var_bss;
}

int32_t *get_lib_var_data() {
    return &lib_var_data;
}
