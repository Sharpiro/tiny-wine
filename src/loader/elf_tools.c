#include "elf_tools.h"
#include "../tiny_c/tiny_c.h"
#include "loader_lib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define ELF_HEADER_LEN sizeof(ELF_HEADER)

const uint8_t ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

// @todo: finds by 'type' which is not unique
static SECTION_HEADER *find_section_header(
    SECTION_HEADER *section_headers, size_t len, size_t type
) {
    SECTION_HEADER *dyn_sym_section_header = NULL;
    for (size_t i = 0; i < len; i++) {
        SECTION_HEADER *curr_header = &section_headers[i];
        if (curr_header->sh_type == type) {
            dyn_sym_section_header = curr_header;
            break;
        }
    }

    return dyn_sym_section_header;
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

    SECTION_HEADER *bss_section_header =
        find_section_header(section_headers, elf_header.e_shnum, SHT_NOBITS);

    SECTION_HEADER *dyn_sym_section_header =
        find_section_header(section_headers, elf_header.e_shnum, SHT_DYNSYM);
    if (dyn_sym_section_header != NULL) {
        size_t dyn_sym_section_size = dyn_sym_section_header->sh_size;
        // size_t dyn_symbols_len =
        //     dyn_sym_section_header->sh_size /
        //     dyn_sym_section_header->sh_entsize;
        SYMBOL *dyn_symbols = tinyc_malloc_arena(dyn_sym_section_size);
        seeked =
            tinyc_lseek(fd, (off_t)dyn_sym_section_header->sh_addr, SEEK_SET);
        if (seeked != (off_t)dyn_sym_section_header->sh_addr) {
            BAIL("seek failed");
        }
        ssize_t dyn_sym_read_len =
            tiny_c_read(fd, dyn_symbols, dyn_sym_section_size);
        if ((size_t)dyn_sym_read_len != dyn_sym_section_size) {
            BAIL("read failed\n")
        }
    }

    SECTION_HEADER *dyn_str_section_header =
        find_section_header(section_headers, elf_header.e_shnum, SHT_STRTAB);

    if (dyn_str_section_header != NULL) {
        // uint8_t data = tinyc_malloc_arena(dyn_)
    }

    *elf_data = (struct ElfData){
        .header = elf_header,
        .program_headers = program_headers,
        .program_headers_len = elf_header.e_phnum,
        .bss_section_header = bss_section_header,
        .dyn_sym_section_header = dyn_sym_section_header,
        .dyn_symbols = NULL,
        .dyn_symbols_len = 0,
    };

    return true;
}
