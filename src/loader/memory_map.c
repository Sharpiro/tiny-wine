#include "memory_map.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include <stdint.h>

bool get_memory_regions(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    struct MemoryRegion **memory_regions_ptr,
    size_t *memory_regions_len
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
            tiny_c_fprintf(
                STDERR, "WARNING: dynamic relocations not supported\n"
            );
            continue;
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
            tiny_c_fprintf(
                STDERR, "WARNING: non-zero file offset memory region\n"
            );
            end += 0x1000;
        }

        memory_regions[j++] = (struct MemoryRegion){
            .start = start,
            .end = end,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
    }

    *memory_regions_len = j;
    return true;
}
