#pragma once

#include <elf.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef ARM32

#define ELF_HEADER Elf32_Ehdr
#define PROGRAM_HEADER Elf32_Phdr
#define SECTION_HEADER Elf32_Shdr
#define SYMBOL Elf32_Sym

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
    size_t program_headers_len;
    SECTION_HEADER *bss_section_header;
    SECTION_HEADER *dyn_sym_section_header;
    SYMBOL *dyn_symbols;
    size_t dyn_symbols_len;
};

bool get_elf_data(int fd, struct ElfData *elf_data);
