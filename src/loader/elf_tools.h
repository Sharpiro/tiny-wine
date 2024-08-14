#pragma once

#include <elf.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef ARM32

#define ELF_HEADER Elf32_Ehdr
#define PROGRAM_HEADER Elf32_Phdr
#define SECTION_HEADER Elf32_Shdr

#endif

struct MemoryRegion {
    size_t start;
    size_t end;
    size_t file_offset;
    size_t permissions;
};

struct ElfData {
    ELF_HEADER header;
    PROGRAM_HEADER *program_headers;
    SECTION_HEADER bss_section_header;
    struct MemoryRegion *memory_regions;
    size_t memory_regions_len;
};

bool get_elf_data(int fd, struct ElfData *elf_data);
