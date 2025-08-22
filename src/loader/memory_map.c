#include <dlls/twlibc.h>
#include <dlls/twlibc_linux.h>
#include <loader/memory_map.h>
#include <log.h>

#define LOADER_BUFFER_LEN 0x210'000

static uint8_t *loader_buffer = NULL;
static size_t loader_heap_index = 0;

void *loader_malloc_arena(size_t n) {
    if (loader_buffer == NULL) {
        loader_buffer = mmap(
            NULL,
            LOADER_BUFFER_LEN,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );
        if (loader_buffer == MAP_FAILED) {
            return NULL;
        }
    }

    const size_t POINTER_SIZE = sizeof(size_t);
    size_t alignment_mod = n % POINTER_SIZE;
    size_t aligned_end =
        alignment_mod == 0 ? n : n + POINTER_SIZE - alignment_mod;
    if (loader_heap_index + aligned_end > LOADER_BUFFER_LEN) {
        LOGERROR("size exceeded\n");
        return NULL;
    }

    void *address = (void *)(loader_buffer + loader_heap_index);
    loader_heap_index += aligned_end;

    return address;
}

bool reserve_region_space(MemoryRegionList *regions, size_t *reserved_address) {
    size_t reserved_len = 0;
    for (size_t i = 0; i < regions->length; i++) {
        const struct MemoryRegion *region = &regions->data[i];
        reserved_len += region->end - region->start;
    }

    uint8_t *addr = mmap(
        NULL,
        reserved_len,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (addr == MAP_FAILED) {
        BAIL(
            "failed trying to reserve mapped region, errorno %d: %s\n",
            errno,
            strerror(errno)
        );
    }
    if (munmap(addr, reserved_len) == -1) {
        BAIL(
            "failed trying to reserve mapped region, errorno %d: %s\n",
            errno,
            strerror(errno)
        );
    }

    *reserved_address = (size_t)addr;
    for (size_t i = 0; i < regions->length; i++) {
        struct MemoryRegion *memory_region = &regions->data[i];
        memory_region->start += *reserved_address;
        memory_region->end += *reserved_address;
    }

    return true;
}

bool map_memory_regions(
    int32_t fd, const struct MemoryRegion *regions, size_t regions_len
) {
    for (size_t i = 0; i < regions_len; i++) {
        const struct MemoryRegion *memory_region = &regions[i];
        size_t memory_region_len = memory_region->end - memory_region->start;
        int32_t prot_read = (memory_region->permissions & 4) >> 2;
        int32_t prot_write = memory_region->permissions & 2;
        int32_t prot_execute = (int32_t)(memory_region->permissions & 1) << 2;
        int32_t map_protection = prot_read | prot_write | prot_execute;
        LOGDEBUG(
            "mapping address: %zx:%zx, offset: %zx, permissions: %zd\n",
            memory_region->start,
            memory_region->end,
            memory_region->file_offset,
            memory_region->permissions
        );

        int32_t map_anonymous =
            memory_region->is_direct_file_map ? 0 : MAP_ANONYMOUS;
        off_t file_offset = memory_region->is_direct_file_map
            ? (off_t)memory_region->file_offset
            : 0;
        int32_t flags = MAP_PRIVATE | MAP_FIXED | map_anonymous;
        struct utsname name;
        uname(&name);
        if (*name.release >= '5') {
            flags |= MAP_FIXED_NOREPLACE;
        }
        uint8_t *addr = mmap(
            (void *)memory_region->start,
            memory_region_len,
            map_protection,
            flags,
            fd,
            file_offset
        );
        if ((size_t)addr != memory_region->start) {
            BAIL(
                "failed trying to map %zx, errorno %d: %s\n",
                memory_region->start,
                errno,
                strerror(errno)
            );
        }
    }

    return true;
}
