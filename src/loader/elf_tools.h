#pragma once

#include <elf.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef ARM32

#define ELF_HEADER Elf32_Ehdr
#define PROGRAM_HEADER Elf32_Phdr
#define SECTION_HEADER Elf32_Shdr
#define SYMBOL Elf32_Sym
#define RELOCATION Elf32_Rel

#endif

struct GlobalOffsetTableEntry {
    size_t index;
    size_t value;
};

struct SectionHeader {
    const char *name;
    size_t type;
    size_t addr;
    size_t offset;
    size_t size;
    size_t entry_size;
};

struct Symbol {
    const char *name;
    size_t value;
    size_t size;
    size_t type;
    size_t binding;
};

struct Relocation {
    size_t offset;
    size_t type;
    struct Symbol symbol;
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
    struct GlobalOffsetTableEntry *got_entries;
    size_t got_len;
};

struct ElfData {
    ELF_HEADER header;
    PROGRAM_HEADER *program_headers;
    size_t program_headers_len;
    struct SectionHeader *section_headers;
    size_t section_headers_len;
    struct DynamicData *dynamic_data;
};

bool get_elf_data(int fd, struct ElfData *elf_data);

const struct SectionHeader *find_section_header(
    const struct SectionHeader *section_headers, size_t len, const char *name
);
