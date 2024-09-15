#include "loader_lib.h"
#include "../tiny_c/tiny_c.h"
#include <stddef.h>
#include <sys/mman.h>

int32_t loader_log_handle = STDERR;

static uint8_t *loader_buffer = NULL;
static size_t loader_heap_index = 0;

void *loader_malloc_arena(size_t n) {
    if (loader_buffer == NULL) {
        loader_buffer = tiny_c_mmap(
            LOADER_BUFFER_ADDRESS,
            LOADER_BUFFER_LEN,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
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
        BAIL("relocation_name was null");
    }
    if (runtime_symbols == NULL) {
        BAIL("runtime_symbols was null");
    }
    if (relocation_address == NULL) {
        BAIL("relocation_address was null");
    }

    for (size_t j = 0; j < runtime_symbols_len; j++) {
        const struct RuntimeSymbol *curr_runtime_symbol = &runtime_symbols[j];
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
