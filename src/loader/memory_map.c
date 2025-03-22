#include "memory_map.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include <stdint.h>
#include <sys/mman.h>

bool get_memory_regions(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    size_t address_offset,
    MemoryRegionList *memory_regions
) {
    if (program_headers == NULL) {
        BAIL("program_headers cannot be null\n");
    }
    if (memory_regions == NULL) {
        BAIL("memory_regions cannot be null\n");
    }

    for (size_t i = 0; i < program_headers_len; i++) {
        const PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        if (program_header->p_filesz == 0 && program_header->p_offset != 0) {
            LOGINFO(
                "PH %d: zero filesize w/ non-zero offset may not be "
                "unsupported\n",
                i + 1
            );
        }
        if (program_header->p_memsz != program_header->p_filesz) {
            LOGINFO("PH filesize != memsize\n", i + 1);
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
            program_header->p_vaddr + program_header->p_memsz;
        if (max_region_address > end) {
            LOGTRACE("Memory region %x extended due to offset\n", start);
            end += 0x1000;
        }

        start += address_offset;
        end += address_offset;

        struct MemoryRegion memory_region = (struct MemoryRegion){
            .start = start,
            .end = end,
            .is_direct_file_map = program_header->p_filesz > 0,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
        MemoryRegionList_add(memory_regions, memory_region);
    }

    return true;
}

bool get_reserved_region_space(
    const struct MemoryRegion *regions,
    size_t regions_len,
    uint8_t **reserved_address
) {
    size_t reserved_len = 0;
    for (size_t i = 0; i < regions_len; i++) {
        const struct MemoryRegion *region = &regions[i];
        reserved_len += region->end - region->start;
    }

    uint8_t *addr = tiny_c_mmap(
        0,
        reserved_len,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (addr == MAP_FAILED) {
        BAIL(
            "failed trying to reserve mapped region, errorno %d: %s\n",
            tinyc_errno,
            tinyc_strerror(tinyc_errno)
        );
    }
    if (tiny_c_munmap(addr, reserved_len) == -1) {
        BAIL(
            "failed trying to reserve mapped region, errorno %d: %s\n",
            tinyc_errno,
            tinyc_strerror(tinyc_errno)
        );
    }

    *reserved_address = addr;
    return true;
}

bool map_memory_regions(
    int32_t fd, const struct MemoryRegion *regions, size_t regions_len
) {
    for (size_t i = 0; i < regions_len; i++) {
        const struct MemoryRegion *memory_region = &regions[i];
        size_t memory_region_len = memory_region->end - memory_region->start;
        size_t prot_read = (memory_region->permissions & 4) >> 2;
        size_t prot_write = memory_region->permissions & 2;
        size_t prot_execute = (memory_region->permissions & 1) << 2;
        size_t map_protection = prot_read | prot_write | prot_execute;
        LOGINFO(
            "mapping address: %x:%x, offset: %x, permissions: %d\n",
            memory_region->start,
            memory_region->end,
            memory_region->file_offset,
            memory_region->permissions
        );

        size_t map_anonymous =
            memory_region->is_direct_file_map ? 0 : MAP_ANONYMOUS;
        size_t file_offset =
            memory_region->is_direct_file_map ? memory_region->file_offset : 0;
        size_t flags = MAP_PRIVATE | MAP_FIXED | map_anonymous;
        struct utsname name;
        tinyc_uname(&name);
        if (*name.release >= '5') {
            flags |= MAP_FIXED_NOREPLACE;
        }
        uint8_t *addr = tiny_c_mmap(
            memory_region->start,
            memory_region_len,
            map_protection,
            flags,
            fd,
            file_offset
        );
        if ((size_t)addr != memory_region->start) {
            BAIL(
                "failed trying to map %x, errorno %d: %s\n",
                memory_region->start,
                tinyc_errno,
                tinyc_strerror(tinyc_errno)
            );
        }
    }

    return true;
}
