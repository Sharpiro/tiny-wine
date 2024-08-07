#include "elf_tools.h"
#include "tiny_c/tiny_c.h"

#define ELF_HEADER_LEN sizeof(ELF_HEADER)

uint8_t ELF_HEADER_BUFFER[ELF_HEADER_LEN] = {0};
uint8_t PROGRAM_HEADERS_BUFFER[1000] = {0};
uint8_t MEMORY_REGIONS_BUFFER[1000] = {0};

const uint8_t ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

#define BAIL(fmt, ...)                                                         \
    tiny_c_fprintf(STDERR, fmt, ##__VA_ARGS__);                                \
    return false;

#define EXAMPLE2(fmt, ...) tiny_c_fprintf(STDERR, fmt, ##__VA_ARGS__)

bool get_elf_data(int fd, struct ElfData *elf_data) {
    ssize_t header_read_len =
        tiny_c_read(fd, ELF_HEADER_BUFFER, ELF_HEADER_LEN);
    if (header_read_len != ELF_HEADER_LEN) {
        tiny_c_fprintf(STDERR, "read failed\n");
        return false;
    }

    ELF_HEADER *elf_header = (ELF_HEADER *)ELF_HEADER_BUFFER;
    if (tiny_c_memcmp(elf_header->e_ident, ELF_MAGIC, 4)) {
        tiny_c_fprintf(STDERR, "Program type not supported\n");
        return false;
    }

    if (elf_header->e_phoff != ELF_HEADER_LEN) {
        BAIL("file seek not implemented\n")
    }

    size_t program_headers_len = elf_header->e_phnum * elf_header->e_phentsize;
    ssize_t ph_read_len =
        tiny_c_read(fd, PROGRAM_HEADERS_BUFFER, program_headers_len);
    if ((size_t)ph_read_len != program_headers_len) {
        BAIL("read failed\n")
    }

    PROGRAM_HEADER *program_headers =
        (PROGRAM_HEADER *)(PROGRAM_HEADERS_BUFFER);

    // @todo: malloc
    struct MemoryRegion *memory_regions =
        (struct MemoryRegion *)MEMORY_REGIONS_BUFFER;

    size_t j = 0;
    for (size_t i = 0; i < elf_header->e_phnum; i++) {
        PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }

        uint32_t file_offset = program_header->p_offset /
                               program_header->p_align *
                               program_header->p_align;
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

    *elf_data = (struct ElfData){
        .header = elf_header,
        .program_headers = program_headers,
        .memory_regions = memory_regions,
        .memory_regions_len = j,
    };

    return true;
}
