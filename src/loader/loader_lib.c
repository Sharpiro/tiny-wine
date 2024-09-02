#include "loader_lib.h"
#include "../tiny_c/tiny_c.h"
#include <stddef.h>
#include <sys/mman.h>

#define LOADER_BUFFER_ADDRESS 0x7d7e0000
#define LOADER_BUFFER_LEN 0x2000

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

    size_t pointer_size = sizeof(size_t);
    size_t alignment_mod = n % pointer_size;
    size_t aligned_end =
        alignment_mod == 0 ? n : n + pointer_size - alignment_mod;
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
