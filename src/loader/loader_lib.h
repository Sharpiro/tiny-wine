#pragma once

#include <stdint.h>
#include <stdlib.h>

void *loader_malloc_arena(size_t n);

void loader_free_arena(void);
