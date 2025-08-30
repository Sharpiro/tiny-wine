#include <dlls/ntdll.h>
#include <sys_linux.h>

#define STDOUT 1
#define STDERR 2

void DllMainCRTStartup(void) {
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"

void *memset(void *s_buffer, int c_value, size_t n_count) {
    for (size_t i = 0; i < n_count; i++) {
        ((uint8_t *)s_buffer)[i] = (uint8_t)c_value;
    }
    return s_buffer;
}

#pragma clang diagnostic pop

int32_t ntdll_large_params_ntdll(
    int32_t one,
    int32_t two,
    int32_t three,
    int32_t four,
    int32_t five,
    int32_t six,
    int32_t seven,
    int32_t eight
) {
    return one + two + three + four + five + six + seven + eight;
}
