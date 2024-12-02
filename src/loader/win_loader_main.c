#include "../tiny_c/tiny_c.h"
#include "./pe_tools.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        return -1;
    }

    char *filename = argv[1];
    LOADER_LOG("Starting loader, %s, %d\n", filename, argc);

    int32_t pid = tiny_c_get_pid();
    LOADER_LOG("pid: %d\n", pid);

    // @todo: unmaps readonly section of loader elf
    if (tiny_c_munmap(0x400000, 0x1000)) {
        tiny_c_fprintf(STDERR, "munmap of self failed\n");
        return -1;
    }

    int32_t fd = tiny_c_open(filename, O_RDONLY);
    if (fd < 0) {
        tiny_c_fprintf(
            STDERR,
            "file error, %d, %s\n",
            tinyc_errno,
            tinyc_strerror(tinyc_errno)
        );
        return -1;
    }

    struct PeData pe_data;
    if (!get_pe_data(fd, &pe_data)) {
        tiny_c_fprintf(STDERR, "error parsing pe data\n");
        return -1;
    }

    struct MemoryRegion memory_regions[] = {{
        .start = 0x140001000,
        .end = 0x140002000,
        .is_file_map = false,
        .file_offset = 0,
        .permissions = 4 | 2 | 1,
    }};
    struct MemoryRegionsInfo memory_regions_info = {
        .start = 0,
        .end = 0,
        .regions = memory_regions,
        .regions_len = 1,
    };
    if (!map_memory_regions(fd, &memory_regions_info)) {
        tiny_c_fprintf(STDERR, "loader map memory regions failed\n");
        return -1;
    }

    uint8_t *temp_region_start = (uint8_t *)0x140001000;
    tinyc_lseek(fd, 0x400, SEEK_SET);
    tiny_c_read(fd, temp_region_start, 72);

    print_memory_regions();
}
