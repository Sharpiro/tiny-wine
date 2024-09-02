#pragma once

#include "elf_tools.h"
#include <stdint.h>

bool get_memory_regions(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    struct MemoryRegion **memory_regions_ptr,
    size_t *memory_regions_len
);

bool map_memory_regions(
    int32_t fd,
    const struct MemoryRegion *memory_regions,
    size_t memory_regions_len
);
