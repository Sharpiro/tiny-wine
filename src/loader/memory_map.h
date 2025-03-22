#pragma once

#include "elf_tools.h"
#include <stdint.h>

bool get_memory_regions_arm(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    size_t address_offset,
    MemoryRegionList *memory_regions
);

bool get_memory_regions(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    MemoryRegionList *memory_regions
);

bool reserve_region_space(MemoryRegionList *regions, size_t *reserved_address);

bool map_memory_regions(
    int32_t fd, const struct MemoryRegion *regions, size_t regions_len
);
