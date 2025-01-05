#include "ntdll.h"
#include <stdint.h>

#define STDOUT 1
#define STDERR 2

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"

#define IS_SIGNED(type) (((type) - 1) < 0) ? true : false

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

double pow(double x, double y);
