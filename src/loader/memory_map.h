#include "elf_tools.h"
#include <stdint.h>

bool get_memory_regions(
    struct ElfData *elf_data, struct MemoryRegion **memory_regions, size_t *len
);
