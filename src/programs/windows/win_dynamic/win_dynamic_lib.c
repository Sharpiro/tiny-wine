#include "./win_dynamic_lib.h"
#include "../../../dlls/msvcrt.h"
#include <stdbool.h>
#include <stdint.h>

uint64_t lib_var_bss = 0;
uint64_t lib_var_data = 42;

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

uint64_t *get_lib_var_bss() {
    return &lib_var_bss;
}

uint64_t *get_lib_var_data() {
    return &lib_var_data;
}

int large(int a, int b, int c, int d, int e, int f, int g, int h) {
    return large_ntdll(a, b, c, d, e, f, g, h);
}
