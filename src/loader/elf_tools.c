#include "elf_tools.h"
#include "../tiny_c/tiny_c.h"
#include "loader_lib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define ELF_HEADER_LEN sizeof(ELF_HEADER)

const uint8_t ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

// @todo: finds by 'type' which is not unique
static const SECTION_HEADER *find_section_header(
    const SECTION_HEADER *section_headers, size_t len, size_t type
) {
    const SECTION_HEADER *dyn_sym_section_header = NULL;
    for (size_t i = 0; i < len; i++) {
        const SECTION_HEADER *curr_header = &section_headers[i];
        if (curr_header->sh_type == type) {
            dyn_sym_section_header = curr_header;
            break;
        }
    }

    return dyn_sym_section_header;
}

static bool get_dynamic_data(
    const SECTION_HEADER *section_headers,
    size_t section_headers_len,
    int32_t fd,
    struct DynamicData *dynamic_data
) {
    if (dynamic_data == NULL) {
        BAIL("dynamic_data was null\n");
    }
    const SECTION_HEADER *dyn_sym_section_header =
        find_section_header(section_headers, section_headers_len, SHT_DYNSYM);
    const SECTION_HEADER *dyn_str_section_header =
        find_section_header(section_headers, section_headers_len, SHT_STRTAB);
    if (dyn_sym_section_header == NULL) {
        return NULL;
    }
    if (dyn_str_section_header == NULL) {
        BAIL("Could not find dynamic string section header\n");
    }

    size_t dyn_sym_section_size = dyn_sym_section_header->sh_size;
    SYMBOL *dyn_symbols = tinyc_malloc_arena(dyn_sym_section_size);
    off_t seeked =
        tinyc_lseek(fd, (off_t)dyn_sym_section_header->sh_addr, SEEK_SET);
    if (seeked != (off_t)dyn_sym_section_header->sh_addr) {
        BAIL("seek failed\n");
    }
    ssize_t read_len = tiny_c_read(fd, dyn_symbols, dyn_sym_section_size);
    if ((size_t)read_len != dyn_sym_section_size) {
        BAIL("read failed\n")
    }

    uint8_t *dyn_strings = tinyc_malloc_arena(dyn_str_section_header->sh_size);
    seeked = tinyc_lseek(fd, (off_t)dyn_str_section_header->sh_addr, SEEK_SET);
    if (seeked != (off_t)dyn_str_section_header->sh_addr) {
        BAIL("seek failed\n");
    }
    read_len = tiny_c_read(fd, dyn_strings, dyn_str_section_header->sh_size);
    if ((size_t)read_len != dyn_str_section_header->sh_size) {
        BAIL("read failed\n")
    }

    size_t dyn_str_table_len =
        dyn_sym_section_header->sh_size / dyn_sym_section_header->sh_entsize;
    struct Symbol *symbols =
        tinyc_malloc_arena(sizeof(struct Symbol) * dyn_str_table_len);

    size_t dyn_symbols_len = 0;
    for (size_t i = 0; i < dyn_str_table_len; i++) {
        SYMBOL dyn_elf_symbol = dyn_symbols[i];
        if (dyn_elf_symbol.st_name == 0) {
            continue;
        }

        char *name = (char *)dyn_strings + dyn_elf_symbol.st_name;
        struct Symbol symbol = {
            .name = name,
            .value = dyn_elf_symbol.st_value,
            .size = dyn_elf_symbol.st_size,
            // @todo: compute this
            .type = 0,
            .bind = 0,
        };
        symbols[dyn_symbols_len++] = symbol;
    }

    *dynamic_data = (struct DynamicData){
        .symbols = symbols,
        .symbols_len = dyn_symbols_len,
    };

    return true;
}

bool get_elf_data(int fd, struct ElfData *elf_data) {
    ELF_HEADER elf_header;
    ssize_t header_read_len = tiny_c_read(fd, &elf_header, ELF_HEADER_LEN);
    if (header_read_len != ELF_HEADER_LEN) {
        tiny_c_fprintf(STDERR, "read failed\n");
        return false;
    }

    if (tiny_c_memcmp(elf_header.e_ident, ELF_MAGIC, 4)) {
        tiny_c_fprintf(STDERR, "Program type not supported\n");
        return false;
    }

    if (elf_header.e_phoff != ELF_HEADER_LEN) {
        BAIL("expected program headers after elf header\n")
    }

    size_t program_headers_len = elf_header.e_phnum * elf_header.e_phentsize;
    PROGRAM_HEADER *program_headers = loader_malloc_arena(program_headers_len);
    ssize_t ph_read_len = tiny_c_read(fd, program_headers, program_headers_len);
    if ((size_t)ph_read_len != program_headers_len) {
        BAIL("read failed\n")
    }

    size_t section_headers_size = elf_header.e_shnum * elf_header.e_shentsize;
    off_t seeked = tinyc_lseek(fd, (off_t)elf_header.e_shoff, SEEK_SET);
    if (seeked != (off_t)elf_header.e_shoff) {
        BAIL("seek failed");
    }
    SECTION_HEADER *section_headers = loader_malloc_arena(program_headers_len);
    ssize_t sh_read_len =
        tiny_c_read(fd, section_headers, section_headers_size);
    if ((size_t)sh_read_len != section_headers_size) {
        BAIL("read failed\n")
    }

    const SECTION_HEADER *bss_section_header =
        find_section_header(section_headers, elf_header.e_shnum, SHT_NOBITS);

    struct DynamicData *dynamic_data =
        tinyc_malloc_arena(sizeof(struct DynamicData));
    if (!get_dynamic_data(
            section_headers, elf_header.e_shnum, fd, dynamic_data
        )) {
        BAIL("failed getting dynamic data\n")
    }

    *elf_data = (struct ElfData){
        .header = elf_header,
        .program_headers = program_headers,
        .program_headers_len = elf_header.e_phnum,
        .bss_section_header = bss_section_header,
        .dynamic_data = dynamic_data,
    };

    return true;
}
