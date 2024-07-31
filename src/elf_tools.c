#include "elf_tools.h"
#include <stdint.h>
#include <tiny_c.h>

#define ELF_HEADER_LEN sizeof(ELF_HEADER)

uint8_t JERRY_RIGGED_MALLOC_BUFFER[100] = {0};

const uint8_t ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

bool get_elf_data(int fd, struct ElfData *elf_data) {
    uint8_t elf_header_buffer[ELF_HEADER_LEN];
    ssize_t read_result = tiny_c_read(fd, elf_header_buffer, ELF_HEADER_LEN);
    if (read_result != ELF_HEADER_LEN) {
        tiny_c_fprintf(STDERR, "read inferior header failed\n");
        return false;
    }

    ELF_HEADER *elf_header = (ELF_HEADER *)elf_header_buffer;
    if (tiny_c_memcmp(elf_header->e_ident, ELF_MAGIC, 4)) {
        tiny_c_fprintf(STDERR, "Program type not supported\n");
        return false;
    }

    // @todo: load program headers data
    return false;

    // @todo: malloc
    struct MemoryRegion *memory_regions =
        (struct MemoryRegion *)JERRY_RIGGED_MALLOC_BUFFER;

    PROGRAM_HEADER *program_headers =
        (PROGRAM_HEADER *)(elf_header_buffer + elf_header->e_phoff);

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

    *elf_data = (struct ElfData){
        .header = elf_header,
        .program_headers = program_headers,
        .memory_regions = memory_regions,
        .memory_regions_len = j,
    };

    return true;
}
