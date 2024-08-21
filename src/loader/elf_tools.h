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

struct Symbol {
    char *name;
    size_t value;
    size_t size;
    size_t type;
    size_t bind;
};

struct MemoryRegion {
    size_t start;
    size_t end;
    size_t file_offset;
    size_t permissions;
};

struct DynamicData {
    struct Symbol *symbols;
    size_t symbols_len;
};

struct ElfData {
    ELF_HEADER header;
    PROGRAM_HEADER *program_headers;
    size_t program_headers_len;
    const SECTION_HEADER *bss_section_header;
    struct DynamicData *dynamic_data;
};

bool get_elf_data(int fd, struct ElfData *elf_data);
