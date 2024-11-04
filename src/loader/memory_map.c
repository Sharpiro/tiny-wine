#include "memory_map.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include <stdint.h>
#include <sys/mman.h>

// @todo: do not want 2 of these
bool get_memory_regions_info_arm(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    size_t address_offset,
    struct MemoryRegionsInfo *memory_regions_info
) {
    if (program_headers == NULL) {
        BAIL("program_headers cannot be null\n");
    }
    if (memory_regions_info == NULL) {
        BAIL("memory_regions_info cannot be null\n");
    }

    struct MemoryRegion *memory_regions =
        loader_malloc_arena(sizeof(struct MemoryRegion) * program_headers_len);

    size_t j = 0;
    size_t regions_info_start = 0;
    size_t regions_info_end = 0;
    for (size_t i = 0; i < program_headers_len; i++) {
        const PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        if (program_header->p_filesz == 0 && program_header->p_offset != 0) {
            BAIL(
                "PH %d: zero filesize w/ non-zero offset unsupported\n", i + 1
            );
        }

        size_t file_offset = program_header->p_offset /
            program_header->p_align * program_header->p_align;
        size_t start = program_header->p_vaddr / program_header->p_align *
            program_header->p_align;
        size_t end = start +
            program_header->p_memsz / program_header->p_align *
                program_header->p_align +
            program_header->p_align;

        size_t max_region_address =
            start + program_header->p_offset + program_header->p_memsz;
        if (max_region_address > end) {
            LOADER_LOG(
                "WARNING: memory region %x extended due to offset\n", start
            );
            end += 0x1000;
        }

        start = start + address_offset;
        end = end + address_offset;
        regions_info_end = end;
        if (regions_info_start == 0) {
            regions_info_start = start;
        }

        memory_regions[j++] = (struct MemoryRegion){
            .start = start,
            .end = end,
            .is_file_map = program_header->p_filesz > 0,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
    }

    *memory_regions_info = (struct MemoryRegionsInfo){
        .start = regions_info_start,
        .end = regions_info_end,
        .regions = memory_regions,
        .regions_len = j,
    };
    return true;
}

bool get_memory_regions_info_x86(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    size_t _,
    struct MemoryRegionsInfo *memory_regions_info
) {
    if (program_headers == NULL) {
        BAIL("program_headers cannot be null\n");
    }
    if (memory_regions_info == NULL) {
        BAIL("memory_regions_info cannot be null\n");
    }

    struct MemoryRegion *memory_regions =
        loader_malloc_arena(sizeof(struct MemoryRegion) * program_headers_len);

    size_t regions_start = 0;
    size_t regions_end = 0;
    size_t j = 0;
    for (size_t i = 0; i < program_headers_len; i++) {
        const PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        if (program_header->p_filesz == 0 && program_header->p_offset != 0) {
            BAIL(
                "PH %d: zero filesize w/ non-zero offset unsupported\n", i + 1
            );
        }

        size_t file_offset = program_header->p_offset /
            program_header->p_align * program_header->p_align;
        size_t start = program_header->p_vaddr / program_header->p_align *
            program_header->p_align;
        size_t end = start +
            program_header->p_memsz / program_header->p_align *
                program_header->p_align +
            program_header->p_align;

        size_t max_region_address =
            start + program_header->p_offset + program_header->p_memsz;
        if (max_region_address > end) {
            LOADER_LOG(
                "WARNING: memory region %x extended due to offset\n", start
            );
        }

        if (j == 0) {
            regions_start = start;
        }
        regions_end = end;

        memory_regions[j++] = (struct MemoryRegion){
            .start = start,
            .end = end,
            .is_file_map = program_header->p_filesz > 0,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
    }

    *memory_regions_info = (struct MemoryRegionsInfo){
        .start = regions_start,
        .end = regions_end,
        .regions = memory_regions,
        .regions_len = j,
    };
    return true;
}

bool map_memory_regions(
    int32_t fd, const struct MemoryRegionsInfo *memory_regions_info
) {
    for (size_t i = 0; i < memory_regions_info->regions_len; i++) {
        const struct MemoryRegion *memory_region =
            &memory_regions_info->regions[i];
        size_t memory_region_len = memory_region->end - memory_region->start;
        size_t prot_read = (memory_region->permissions & 4) >> 2;
        size_t prot_write = memory_region->permissions & 2;
        size_t prot_execute = (memory_region->permissions & 1) << 2;
        size_t map_protection = prot_read | prot_write | prot_execute;
        LOADER_LOG(
            "mapping address: %x:%x, permissions: %x\n",
            memory_region->start,
            memory_region->end,
            memory_region->permissions
        );
        if (!memory_region->is_file_map) {
            LOADER_LOG("WARNING: non-file map unsupported\n");
        }

        size_t map_anonymous = memory_region->is_file_map ? 0 : MAP_ANONYMOUS;
        uint8_t *addr = tiny_c_mmapx86(
            memory_region->start,
            memory_region_len,
            map_protection,
            MAP_PRIVATE | MAP_FIXED | map_anonymous,
            fd,
            memory_region->file_offset
        );
        if ((size_t)addr != memory_region->start) {
            BAIL(
                "map failed, %x, %s\n", tinyc_errno, tinyc_strerror(tinyc_errno)
            );
        }
    }

    return true;
}
