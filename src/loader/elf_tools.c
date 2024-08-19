#include "elf_tools.h"
#include "../tiny_c/tiny_c.h"
#include "loader_lib.h"
#include <stdint.h>
#include <stdio.h>

#define ELF_HEADER_LEN sizeof(ELF_HEADER)

const uint8_t ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

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

    size_t section_headers_len = elf_header.e_shnum * elf_header.e_shentsize;
    off_t seeked = tinyc_lseek(fd, (off_t)elf_header.e_shoff, SEEK_SET);
    if (seeked != (off_t)elf_header.e_shoff) {
        BAIL("seek failed");
    }
    SECTION_HEADER *section_headers = loader_malloc_arena(program_headers_len);
    ssize_t sh_read_len = tiny_c_read(fd, section_headers, section_headers_len);
    if ((size_t)sh_read_len != section_headers_len) {
        BAIL("read failed\n")
    }

    SECTION_HEADER *bss_section_header = NULL;
    for (size_t i = 0; i < elf_header.e_shnum; i++) {
        SECTION_HEADER *curr_header = &section_headers[i];
        if (curr_header->sh_type == SHT_NOBITS) {
            bss_section_header = curr_header;
            break;
        }
    }

    struct MemoryRegion *memory_regions =
        loader_malloc_arena(sizeof(struct MemoryRegion) * elf_header.e_phnum);

    size_t j = 0;
    for (size_t i = 0; i < elf_header.e_phnum; i++) {
        PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }

        uint32_t file_offset = program_header->p_offset /
            program_header->p_align * program_header->p_align;
        uint32_t start = program_header->p_vaddr / program_header->p_align *
            program_header->p_align;
        uint32_t end = start +
            program_header->p_memsz / program_header->p_align *
                program_header->p_align +
            program_header->p_align;

        memory_regions[j++] = (struct MemoryRegion){
            .start = start,
            .end = end,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
    }

    if (bss_section_header == NULL) {
        BAIL("could not find .bss section");
    }

    *elf_data = (struct ElfData){
        .header = elf_header,
        .program_headers = program_headers,
        .bss_section_header = *bss_section_header,
        .memory_regions = memory_regions,
        .memory_regions_len = j,
    };

    return true;
}
