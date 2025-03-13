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

int large_params(

    int one,
    int two,
    int three,
    int four,
    int five,
    int six,
    int seven,
    int eight
) {
    return one + two + three + four + five + six + seven + eight;
}
