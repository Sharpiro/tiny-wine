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
#define DYNAMIC_ENTRY Elf32_Dyn
#define RELOCATION_SYMBOL_SHIFT_LENGTH 8
#define FUNCTION_RELOCATION_HEADER ".rel.plt"
#define VARIABLE_RELOCATION_HEADER ".rel.dyn"

#endif

#ifdef AMD64

#define ELF_HEADER Elf64_Ehdr
#define PROGRAM_HEADER Elf64_Phdr
#define SECTION_HEADER Elf64_Shdr
#define SYMBOL Elf64_Sym
#define RELOCATION Elf64_Rela
#define DYNAMIC_ENTRY Elf64_Dyn
#define RELOCATION_SYMBOL_SHIFT_LENGTH 32
#define FUNCTION_RELOCATION_HEADER ".rela.plt"
#define VARIABLE_RELOCATION_HEADER ".rela.dyn"

#endif

struct GotEntry {
    size_t index;
    size_t value;
    bool is_library_virutal_base_address;
    bool is_loader_callback;
    bool is_variable;
    size_t lib_dynamic_offset;
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
    bool is_direct_file_map;
    size_t file_offset;
    size_t file_size;
    size_t permissions;
};

struct MemoryRegionsInfo {
    size_t start;
    size_t end;
    struct MemoryRegion *regions;
    size_t regions_len;
};

struct DynamicData {
    struct Symbol *symbols;
    size_t symbols_len;
    struct GotEntry *got_entries;
    size_t got_entries_len;
    struct Relocation *func_relocations;
    size_t func_relocations_len;
    struct Relocation *var_relocations;
    size_t var_relocations_len;
    char **shared_libraries;
    size_t shared_libraries_len;
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
