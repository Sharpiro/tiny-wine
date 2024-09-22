#pragma once

#include "elf_tools.h"
#include <stdint.h>

bool get_memory_regions_info(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    size_t address_offset,
    struct MemoryRegionsInfo *memory_regions_info
);

bool map_memory_regions(
    int32_t fd,
    const struct MemoryRegion *memory_regions,
    size_t memory_regions_len
);
