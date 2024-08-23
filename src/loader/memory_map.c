#include "memory_map.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include <stdint.h>

bool get_memory_regions(
    struct ElfData *elf_data,
    struct MemoryRegion **memory_regions_ptr,
    size_t *len
) {
    if (memory_regions_ptr == NULL) {
        BAIL("memory_regions cannot be null\n");
    }
    if (len == NULL) {
        BAIL("len cannot be null\n");
    }

    *memory_regions_ptr = loader_malloc_arena(
        sizeof(struct MemoryRegion) * elf_data->header.e_phnum
    );

    size_t j = 0;
    struct MemoryRegion *memory_regions = *memory_regions_ptr;
    for (size_t i = 0; i < elf_data->header.e_phnum; i++) {
        PROGRAM_HEADER *program_header = &elf_data->program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        if (program_header->p_vaddr == 0) {
            BAIL("dynamic relocations not supported\n")
        }

        uint32_t file_offset = program_header->p_offset /
            program_header->p_align * program_header->p_align;
        uint32_t start = program_header->p_vaddr / program_header->p_align *
            program_header->p_align;
        uint32_t end = start +
            program_header->p_memsz / program_header->p_align *
                program_header->p_align +
            program_header->p_align;

        memory_regions[j++] = (struct MemoryRegion){
            .start = start,
            .end = end,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
    }

    *len = j;
    return true;
}
