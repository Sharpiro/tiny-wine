#include "loader_lib.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>

int32_t loader_log_handle = STDERR;

static uint8_t *loader_buffer = NULL;
static size_t loader_heap_index = 0;

void *loader_malloc_arena(size_t n) {
    if (loader_buffer == NULL) {
        loader_buffer = tiny_c_mmapx86(
            LOADER_BUFFER_ADDRESS,
            LOADER_BUFFER_LEN,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
            -1,
            0
        );
        if (loader_buffer == MAP_FAILED) {
            LOADER_LOG("map failed\n");
            return NULL;
        }
    }

    const size_t POINTER_SIZE = sizeof(size_t);
    size_t alignment_mod = n % POINTER_SIZE;
    size_t aligned_end =
        alignment_mod == 0 ? n : n + POINTER_SIZE - alignment_mod;
    if (loader_heap_index + aligned_end > LOADER_BUFFER_LEN) {
        LOADER_LOG("size exceeded\n");
        return NULL;
    }

    void *address = (void *)(loader_buffer + loader_heap_index);
    loader_heap_index += aligned_end;

    return address;
}

void loader_free_arena(void) {
    loader_heap_index = 0;
}

bool find_runtime_relocation(
    const struct RuntimeRelocation *runtime_relocations,
    size_t runtime_relocations_len,
    size_t relocation_offset,
    const struct RuntimeRelocation **runtime_relocation
) {
    if (runtime_relocations == NULL) {
        BAIL("runtime_relocations was null\n");
    }
    if (runtime_relocation == NULL) {
        BAIL("runtime_relocation was null");
    }

    for (size_t j = 0; j < runtime_relocations_len; j++) {
        const struct RuntimeRelocation *curr_relocation =
            &runtime_relocations[j];
        if (curr_relocation->offset == relocation_offset) {
            *runtime_relocation = curr_relocation;
            return true;
        }
    }

    return false;
}

bool get_runtime_address(
    const char *relocation_name,
    const struct RuntimeSymbol *runtime_symbols,
    size_t runtime_symbols_len,
    size_t *relocation_address
) {
    if (relocation_name == NULL) {
        BAIL("relocation_name was null\n");
    }
    if (runtime_symbols == NULL) {
        BAIL("runtime_symbols was null\n");
    }
    if (relocation_address == NULL) {
        BAIL("relocation_address was null\n");
    }

    for (size_t i = 0; i < runtime_symbols_len; i++) {
        const struct RuntimeSymbol *curr_runtime_symbol = &runtime_symbols[i];
        if (curr_runtime_symbol->value == 0) {
            continue;
        }
        if (tiny_c_strcmp(curr_runtime_symbol->name, relocation_name) == 0) {
            *relocation_address = curr_runtime_symbol->value;
            return true;
        }
    }

    return false;
}

bool find_got_entry(
    const struct GotEntry *got_entries,
    size_t got_entries_len,
    size_t offset,
    struct GotEntry **got_entry
) {
    if (got_entries == NULL) {
        BAIL("got_entries was null\n");
    }
    if (got_entry == NULL) {
        BAIL("got_entry was null");
    }

    for (size_t j = 0; j < got_entries_len; j++) {
        const struct GotEntry *curr_got_entry = &got_entries[j];
        if (curr_got_entry->index == offset) {
            *got_entry = (struct GotEntry *)curr_got_entry;
            return true;
        }
    }

    return false;
}

bool read_to_string(const char *path, char **content, size_t size) {
    char *buffer = loader_malloc_arena(size);
    if (buffer == NULL) {
        BAIL("malloc failed\n");
    }

    int32_t fd = tiny_c_open(path, O_RDONLY);
    tiny_c_read(fd, buffer, size);
    tiny_c_close(fd);
    *content = buffer;

    return true;
}

bool print_memory_regions(void) {
    char *maps_buffer;
    if (!read_to_string("/proc/self/maps", &maps_buffer, 0x1000)) {
        BAIL("read failed\n");
    }

    LOADER_LOG("Mapped address regions:\n%s\n", maps_buffer);
    return true;
}
