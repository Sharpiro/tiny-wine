#include "memory_map.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include <stdint.h>
#include <sys/mman.h>

bool get_memory_regions(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    struct MemoryRegion **memory_regions_ptr,
    size_t *memory_regions_len,
    size_t address_offset
) {
    if (program_headers == NULL) {
        BAIL("program_headers cannot be null\n");
    }
    if (memory_regions_ptr == NULL) {
        BAIL("memory_regions cannot be null\n");
    }
    if (memory_regions_len == NULL) {
        BAIL("len cannot be null\n");
    }

    *memory_regions_ptr =
        loader_malloc_arena(sizeof(struct MemoryRegion) * program_headers_len);

    size_t j = 0;
    struct MemoryRegion *memory_regions = *memory_regions_ptr;
    for (size_t i = 0; i < program_headers_len; i++) {
        const PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        if (program_header->p_vaddr == 0) {
            LOADER_LOG("WARNING: dynamic relocation detected\n");
        }

        size_t file_offset = program_header->p_offset /
            program_header->p_align * program_header->p_align;
        size_t start = program_header->p_vaddr / program_header->p_align *
            program_header->p_align;
        size_t end = start +
            program_header->p_memsz / program_header->p_align *
                program_header->p_align +
            program_header->p_align;

        size_t max_region_address =
            start + program_header->p_offset + program_header->p_memsz;
        if (max_region_address > end) {
            LOADER_LOG(
                "WARNING: memory region %x extended due to offset\n", start
            );
            end += 0x1000;
        }

        memory_regions[j++] = (struct MemoryRegion){
            .start = start + address_offset,
            .end = end + address_offset,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
    }

    *memory_regions_len = j;
    return true;
}

bool map_memory_regions(
    int32_t fd,
    const struct MemoryRegion *memory_regions,
    size_t memory_regions_len
) {
    for (size_t i = 0; i < memory_regions_len; i++) {
        const struct MemoryRegion *memory_region = &memory_regions[i];
        size_t memory_region_len = memory_region->end - memory_region->start;
        size_t prot_read = (memory_region->permissions & 4) >> 2;
        size_t prot_write = memory_region->permissions & 2;
        size_t prot_execute = (memory_region->permissions & 1) << 2;
        size_t map_protection = prot_read | prot_write | prot_execute;
        LOADER_LOG("mapping address: %x\n", memory_region->start);
        uint8_t *addr = tiny_c_mmap(
            memory_region->start,
            memory_region_len,
            map_protection,
            MAP_PRIVATE | MAP_FIXED,
            fd,
            memory_region->file_offset
        );
        if ((size_t)addr != memory_region->start) {
            BAIL(
                "map failed, %x, %s\n", tinyc_errno, tinyc_strerror(tinyc_errno)
            );
        }
    }

    return true;
}
