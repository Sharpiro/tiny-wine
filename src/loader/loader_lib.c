#include "loader_lib.h"
#include <stddef.h>

#define LOADER_BUFFER_SIZE 8000

static uint8_t loader_buffer[LOADER_BUFFER_SIZE] = {0};
static size_t loader_heap_end = LOADER_BUFFER_SIZE;
static size_t loader_heap_index = 0;

void *loader_malloc_arena(size_t n) {
    size_t pointer_size = sizeof(size_t);
    size_t alignment_mod = n % pointer_size;
    size_t aligned_end =
        alignment_mod == 0 ? n : n + pointer_size - alignment_mod;
    if (loader_heap_index + aligned_end > loader_heap_end) {
        return NULL;
    }

    void *address = (void *)(loader_buffer + loader_heap_index);
    loader_heap_index += aligned_end;

    return address;
}

void loader_free_arena(void) {
    loader_heap_index = 0;
}
