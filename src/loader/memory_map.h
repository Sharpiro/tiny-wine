#pragma once

#include "list.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct MemoryRegion {
    size_t start;
    size_t end;
    bool is_direct_file_map;
    size_t file_offset;
    size_t file_size;
    size_t permissions;
} MemoryRegion;

CREATE_LIST_STRUCT(MemoryRegion)

void *loader_malloc_arena(size_t n);

bool reserve_region_space(MemoryRegionList *regions, size_t *reserved_address);

bool map_memory_regions(
    int32_t fd, const struct MemoryRegion *regions, size_t regions_len
);
