#include "elf_tools.h"
#include "../tiny_c/tiny_c.h"
#include "loader_lib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ELF_HEADER_LEN sizeof(ELF_HEADER)

const uint8_t ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

static bool get_section_headers(
    const ELF_HEADER *elf_header,
    int32_t fd,
    struct SectionHeader **section_headers_ptr,
    size_t *section_headers_len
) {
    if (section_headers_ptr == NULL) {
        BAIL("section_headers cannot be null\n");
    }
    if (section_headers_len == NULL) {
        BAIL("section_headers_len cannot be null\n");
    }
    off_t seeked = tinyc_lseek(fd, (off_t)elf_header->e_shoff, SEEK_SET);
    if (seeked != (off_t)elf_header->e_shoff) {
        BAIL("seek failed\n");
    }
    size_t section_headers_size = elf_header->e_shnum * elf_header->e_shentsize;
    SECTION_HEADER *elf_section_headers =
        loader_malloc_arena(section_headers_size);
    if (elf_section_headers == NULL) {
        BAIL("malloc failed\n");
    }
    ssize_t read = tiny_c_read(fd, elf_section_headers, section_headers_size);
    if ((size_t)read != section_headers_size) {
        BAIL("read failed\n");
    }

    SECTION_HEADER *shstr_table_header =
        &elf_section_headers[elf_header->e_shstrndx];
    seeked = tinyc_lseek(fd, (off_t)shstr_table_header->sh_offset, SEEK_SET);
    if (seeked != (off_t)shstr_table_header->sh_offset) {
        BAIL("seek failed\n");
    }
    uint8_t *shstr_table = loader_malloc_arena(shstr_table_header->sh_size);
    if (shstr_table == NULL) {
        BAIL("malloc failed\n");
    }
    read = tiny_c_read(fd, shstr_table, shstr_table_header->sh_size);
    if ((size_t)read != shstr_table_header->sh_size) {
        BAIL("read failed\n");
    }

    *section_headers_len = elf_header->e_shnum;
    *section_headers_ptr =
        loader_malloc_arena(sizeof(struct SectionHeader) * elf_header->e_shnum);
    if (section_headers_ptr == NULL) {
        BAIL("malloc failed\n");
    }

    struct SectionHeader *section_headers = *section_headers_ptr;
    for (size_t i = 0; i < elf_header->e_shnum; i++) {
        SECTION_HEADER elf_section_header = elf_section_headers[i];
        char *name = (char *)shstr_table + elf_section_header.sh_name;
        struct SectionHeader section_header = {
            .name = name,
            .type = elf_section_header.sh_type,
            .addr = elf_section_header.sh_addr,
            .offset = elf_section_header.sh_offset,
            .size = elf_section_header.sh_size,
            .entry_size = elf_section_header.sh_entsize,
        };
        section_headers[i] = section_header;
    }

    return true;
}

const struct SectionHeader *find_section_header(
    const struct SectionHeader *section_headers, size_t len, const char *name
) {
    const struct SectionHeader *section_header = NULL;
    for (size_t i = 0; i < len; i++) {
        const struct SectionHeader *curr_header = &section_headers[i];
        if (tiny_c_strcmp(curr_header->name, name) == 0) {
            section_header = curr_header;
            break;
        }
    }

    return section_header;
}

static bool get_dynamic_data(
    const struct SectionHeader *section_headers,
    size_t section_headers_len,
    int32_t fd,
    struct DynamicData **dynamic_data_ptr
) {
    if (dynamic_data_ptr == NULL) {
        BAIL("dynamic_data_ptr was null\n");
    }
    *dynamic_data_ptr = NULL;

    const struct SectionHeader *dyn_sym_section_header =
        find_section_header(section_headers, section_headers_len, ".dynsym");
    const struct SectionHeader *dyn_str_section_header =
        find_section_header(section_headers, section_headers_len, ".dynstr");
    const struct SectionHeader *dynamic_header =
        find_section_header(section_headers, section_headers_len, ".dynamic");
    // @todo: section name: .got vs .got.plt?
    const struct SectionHeader *got_section_header =
        find_section_header(section_headers, section_headers_len, ".got.plt");
    if (dyn_sym_section_header == NULL) {
        return true;
    }
    if (dyn_str_section_header == NULL) {
        BAIL("Could not find .dynstr section header\n");
    }
    if (dynamic_header == NULL) {
        BAIL("Could not find .dynamic section header\n");
    }
    if (got_section_header == NULL) {
        BAIL("Could not find .got section header\n");
    }

    size_t dyn_sym_section_size = dyn_sym_section_header->size;
    SYMBOL *dyn_elf_symbols = loader_malloc_arena(dyn_sym_section_size);
    if (dyn_elf_symbols == NULL) {
        BAIL("malloc failed\n");
    }
    off_t seeked =
        tinyc_lseek(fd, (off_t)dyn_sym_section_header->offset, SEEK_SET);
    if (seeked != (off_t)dyn_sym_section_header->offset) {
        BAIL("seek failed\n");
    }
    ssize_t read = tiny_c_read(fd, dyn_elf_symbols, dyn_sym_section_size);
    if ((size_t)read != dyn_sym_section_size) {
        BAIL("read failed\n");
    }

    char *dyn_strings = loader_malloc_arena(dyn_str_section_header->size);
    if (dyn_strings == NULL) {
        BAIL("malloc failed\n");
    }
    seeked = tinyc_lseek(fd, (off_t)dyn_str_section_header->offset, SEEK_SET);
    if (seeked != (off_t)dyn_str_section_header->offset) {
        BAIL("seek failed\n");
    }
    read = tiny_c_read(fd, dyn_strings, dyn_str_section_header->size);
    if ((size_t)read != dyn_str_section_header->size) {
        BAIL("read failed\n");
    }

    size_t dyn_sym_table_len =
        dyn_sym_section_header->size / dyn_sym_section_header->entry_size;
    struct Symbol *dyn_symbols =
        loader_malloc_arena(sizeof(struct Symbol) * dyn_sym_table_len);
    if (dyn_symbols == NULL) {
        BAIL("malloc failed\n");
    }

    size_t dyn_symbols_len = 0;
    for (size_t i = 0; i < dyn_sym_table_len; i++) {
        SYMBOL dyn_elf_symbol = dyn_elf_symbols[i];
        char *name = dyn_strings + dyn_elf_symbol.st_name;
        size_t type = dyn_elf_symbol.st_info & 0x0f;
        size_t binding = dyn_elf_symbol.st_info >> 4;
        struct Symbol symbol = {
            .name = name,
            .value = dyn_elf_symbol.st_value,
            .size = dyn_elf_symbol.st_size,
            .type = type,
            .binding = binding,
        };
        dyn_symbols[dyn_symbols_len++] = symbol;
    }

    size_t *global_offset_table = loader_malloc_arena(got_section_header->size);
    if (global_offset_table == NULL) {
        BAIL("malloc failed\n");
    }
    seeked = tinyc_lseek(fd, (off_t)got_section_header->offset, SEEK_SET);
    if (seeked != (off_t)got_section_header->offset) {
        BAIL("seek failed\n");
    }
    read = tiny_c_read(fd, global_offset_table, got_section_header->size);
    if ((size_t)read != got_section_header->size) {
        BAIL("read failed\n");
    }

    size_t got_len = got_section_header->size / got_section_header->entry_size;
    if (got_len < 3) {
        BAIL(
            "unsupported GOT length %x, unknown loader callback location\n",
            got_len
        );
    }

    struct GotEntry *got_entries =
        loader_malloc_arena(sizeof(struct GotEntry) * got_len);
    size_t got_base_addr = got_section_header->addr;
    for (size_t i = 0; i < got_len; i++) {
        size_t index = got_base_addr + i * got_section_header->entry_size;
        size_t value = global_offset_table[i];
        struct GotEntry got_entry = {
            .index = index,
            .value = value,
            .is_loader_callback = i == 2,
        };
        got_entries[i] = got_entry;
    }

    /* Load function relocations */
    // @todo: sections name differs per arch?
    const struct SectionHeader *func_reloc_header =
        find_section_header(section_headers, section_headers_len, ".rela.plt");
    struct Relocation *func_relocations = NULL;
    size_t func_relocations_len = 0;
    if (func_reloc_header) {
        RELOCATION *elf_func_relocations =
            loader_malloc_arena(func_reloc_header->size);
        if (elf_func_relocations == NULL) {
            BAIL("malloc failed\n");
        }
        seeked = tinyc_lseek(fd, (off_t)func_reloc_header->offset, SEEK_SET);
        if (seeked != (off_t)func_reloc_header->offset) {
            BAIL("seek failed\n");
        }
        read = tiny_c_read(fd, elf_func_relocations, func_reloc_header->size);
        if ((size_t)read != func_reloc_header->size) {
            BAIL("read failed\n");
        }

        func_relocations_len =
            func_reloc_header->size / func_reloc_header->entry_size;
        func_relocations = loader_malloc_arena(
            sizeof(struct Relocation) * func_relocations_len
        );
        for (size_t i = 0; i < func_relocations_len; i++) {
            RELOCATION *elf_relocation = &elf_func_relocations[i];
            size_t type = elf_relocation->r_info & 0xff;
            size_t symbol_index =
                elf_relocation->r_info >> RELOCATION_SYMBOL_SHIFT_LENGTH;
            struct Symbol symbol = dyn_symbols[symbol_index];
            struct Relocation relocation = {
                .offset = elf_relocation->r_offset,
                .type = type,
                .symbol = symbol,
            };
            func_relocations[i] = relocation;
        }
    }
    /* Load variable relocations */
    const struct SectionHeader *var_reloc_header =
        find_section_header(section_headers, section_headers_len, ".rel.dyn");
    struct Relocation *var_relocations = NULL;
    size_t var_relocations_len = 0;
    if (var_reloc_header) {
        RELOCATION *elf_var_relocations =
            loader_malloc_arena(var_reloc_header->size);
        if (elf_var_relocations == NULL) {
            BAIL("malloc failed\n");
        }
        seeked = tinyc_lseek(fd, (off_t)var_reloc_header->offset, SEEK_SET);
        if (seeked != (off_t)var_reloc_header->offset) {
            BAIL("seek failed\n");
        }
        read = tiny_c_read(fd, elf_var_relocations, var_reloc_header->size);
        if ((size_t)read != var_reloc_header->size) {
            BAIL("read failed\n");
        }

        var_relocations_len =
            var_reloc_header->size / var_reloc_header->entry_size;
        var_relocations = loader_malloc_arena(
            sizeof(struct Relocation) * var_relocations_len
        );
        for (size_t i = 0; i < var_relocations_len; i++) {
            RELOCATION *elf_relocation = &elf_var_relocations[i];
            size_t type = elf_relocation->r_info & 0xff;
            size_t symbol_index = elf_relocation->r_info >> 8;
            struct Symbol symbol = dyn_symbols[symbol_index];
            struct Relocation relocation = {
                .offset = elf_relocation->r_offset,
                .type = type,
                .symbol = symbol,
            };
            var_relocations[i] = relocation;
        }
    }

    /* Load dynamic entries */
    size_t dynamic_entries_len =
        dynamic_header->size / dynamic_header->entry_size;
    DYNAMIC_ENTRY *dynamic_entries = loader_malloc_arena(dynamic_header->size);
    if (dynamic_entries == NULL) {
        BAIL("malloc failed\n");
    }
    seeked = tinyc_lseek(fd, (off_t)dynamic_header->offset, SEEK_SET);
    if (seeked != (off_t)dynamic_header->offset) {
        BAIL("seek failed\n");
    }
    read = tiny_c_read(fd, dynamic_entries, dynamic_header->size);
    if ((size_t)read != dynamic_header->size) {
        BAIL("read failed\n");
    }

    char **shared_libraries =
        loader_malloc_arena(sizeof(char *) * dynamic_entries_len);
    if (shared_libraries == NULL) {
        BAIL("malloc failed\n");
    }
    size_t shared_libraries_len = 0;
    for (size_t i = 0; i < dynamic_entries_len; i++) {
        DYNAMIC_ENTRY *dynamic_entry = &dynamic_entries[i];
        if (dynamic_entry->d_tag != 1) {
            continue;
        }

        shared_libraries[shared_libraries_len++] =
            dyn_strings + dynamic_entry->d_un.d_val;
    }

    *dynamic_data_ptr = loader_malloc_arena(sizeof(struct DynamicData));
    **dynamic_data_ptr = (struct DynamicData){
        .symbols = dyn_symbols,
        .symbols_len = dyn_symbols_len,
        .got_entries = got_entries,
        .got_len = got_len,
        .func_relocations = func_relocations,
        .func_relocations_len = func_relocations_len,
        .var_relocations = var_relocations,
        .var_relocations_len = var_relocations_len,
        .shared_libraries = shared_libraries,
        .shared_libraries_len = shared_libraries_len,
    };

    return true;
}

bool get_elf_data(int fd, struct ElfData *elf_data) {
    ELF_HEADER elf_header;
    ssize_t header_read_len = tiny_c_read(fd, &elf_header, ELF_HEADER_LEN);
    if (header_read_len != ELF_HEADER_LEN) {
        BAIL("read failed\n");
    }
    if (tiny_c_memcmp(elf_header.e_ident, ELF_MAGIC, 4)) {
        BAIL("Program type not supported\n");
    }
    if (elf_header.e_phoff != ELF_HEADER_LEN) {
        BAIL("expected program headers after elf header\n");
    }

    size_t program_headers_size = elf_header.e_phnum * elf_header.e_phentsize;
    PROGRAM_HEADER *program_headers = loader_malloc_arena(program_headers_size);
    if (program_headers == NULL) {
        BAIL("malloc failed\n");
    }
    ssize_t ph_read_len =
        tiny_c_read(fd, program_headers, program_headers_size);
    if ((size_t)ph_read_len != program_headers_size) {
        BAIL("read failed\n");
    }

    struct SectionHeader *section_headers;
    size_t section_headers_len;
    if (!get_section_headers(
            &elf_header, fd, &section_headers, &section_headers_len
        )) {
        BAIL("section headers failed\n");
    }

    struct DynamicData *dynamic_data;
    if (!get_dynamic_data(
            section_headers, elf_header.e_shnum, fd, &dynamic_data
        )) {
        BAIL("failed getting dynamic data\n");
    }

    *elf_data = (struct ElfData){
        .header = elf_header,
        .program_headers = program_headers,
        .program_headers_len = elf_header.e_phnum,
        .section_headers = section_headers,
        .section_headers_len = section_headers_len,
        .dynamic_data = dynamic_data,
    };

    return true;
}
