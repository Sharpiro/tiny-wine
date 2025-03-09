#include "./win_dynamic_lib.h"
#include "../../../dlls/msvcrt.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t lib_var_bss = 0;
uint32_t lib_var_data = 42;

/**
 * Called on stdlib load, unload, etc.
 */
bool DllMain(
    [[maybe_unused]] void *hinstDLL,
    [[maybe_unused]] unsigned long fdwReason,
    [[maybe_unused]] void *lpReserved
) {
    return true;
}

uint32_t *get_lib_var_bss() {
    return &lib_var_bss;
}

uint32_t *get_lib_var_data() {
    return &lib_var_data;
}
