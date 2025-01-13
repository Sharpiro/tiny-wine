#include "ntdll.h"

#define STDOUT 1
#define STDERR 2

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

void exit(int32_t exit_code);

int32_t printf(const char *format, ...);

int32_t puts(const char *data);
