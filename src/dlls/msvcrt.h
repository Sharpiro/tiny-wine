#include "ntdll.h"
#include <stdint.h>

void puts(const char *data);

size_t add_many_msvcrt(
    [[maybe_unused]] size_t one,
    [[maybe_unused]] size_t two,
    [[maybe_unused]] size_t three,
    [[maybe_unused]] size_t four,
    [[maybe_unused]] size_t five,
    [[maybe_unused]] size_t six,
    [[maybe_unused]] size_t seven,
    [[maybe_unused]] size_t eight
);
