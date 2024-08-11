#include "loader_lib.h"

#define LOADER_BUFFER_SIZE 1000

static uint8_t loader_buffer[LOADER_BUFFER_SIZE] = {0};
static size_t loader_heap_end = LOADER_BUFFER_SIZE;
static size_t loader_heap_index = 0;

void *loader_malloc_arena(size_t n) {
    if (loader_heap_index + n > loader_heap_end) {
        return NULL;
    }

    void *address = (void *)(loader_buffer + loader_heap_index);
    loader_heap_index += n;

    return address;
}

void loader_free_arena(void) {
    loader_heap_index = 0;
}
