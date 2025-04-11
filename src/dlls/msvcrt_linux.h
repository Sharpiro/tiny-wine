#pragma once

#include <stddef.h>
#include <stdint.h>

int32_t mprotect(void *address, size_t length, int32_t protection);
