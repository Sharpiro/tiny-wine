#include "elf_tools.h"
#include <stdint.h>

uint8_t JERRY_RIGGED_MALLOC_BUFFER[100] = {0};

struct ElfData get_elf_data(const uint8_t *elf_start) {
    ELF_HEADER *elf_header = (ELF_HEADER *)elf_start;
    PROGRAM_HEADER *program_headers =
        (PROGRAM_HEADER *)(elf_start + elf_header->e_phoff);

    // @todo: malloc
    struct MemoryRegion *memory_regions =
        (struct MemoryRegion *)JERRY_RIGGED_MALLOC_BUFFER;

    int j = 0;
    for (int i = 0; i < elf_header->e_phnum; i++) {
        PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }

        uint32_t start = program_header->p_vaddr / program_header->p_align *
                         program_header->p_align;
        uint32_t end = start +
                       program_header->p_memsz / program_header->p_align *
                           program_header->p_align +
                       program_header->p_align;
        memory_regions[j++] = (struct MemoryRegion){
            .start = start,
            .end = end,
            .permissions = program_header->p_flags,
        };
    }

    return (struct ElfData){
        .header = elf_header,
        .program_headers = program_headers,
        .memory_regions = memory_regions,
        .memory_regions_len = j,
    };
}
