#pragma once

#include "elf_tools.h"
#include <stdint.h>
#include <stdlib.h>

#define LOADER_BUFFER_ADDRESS 0x7d7e0000
#define LOADER_BUFFER_LEN 0x4000
#define LOADER_SHARED_LIB_START LOADER_BUFFER_ADDRESS + LOADER_BUFFER_LEN

extern int32_t loader_log_handle;

void *loader_malloc_arena(size_t n);

void loader_free_arena(void);

#ifdef VERBOSE

#define LOADER_LOG(fmt, ...)                                                   \
    tiny_c_fprintf(loader_log_handle, fmt, ##__VA_ARGS__);

#else

#define LOADER_LOG(fmt, ...)                                                   \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

struct SharedLibrary {
    const char *name;
    size_t dynamic_offset;
    struct ElfData elf_data;
    struct MemoryRegion *memory_regions;
    size_t memory_regions_len;
};

struct RuntimeRelocation {
    struct Relocation relocation;
    size_t mapped_lib_address;
};

struct RuntimeSymbol {
    struct Symbol symbol;
    size_t mapped_lib_address;
};

bool get_runtime_function(
    const struct RuntimeRelocation *runtime_relocations,
    size_t runtime_relocations_len,
    const struct RuntimeSymbol *runtime_symbols,
    size_t runtime_symbols_len,
    size_t relocation_offset,
    size_t *relocation_address,
    const char **relocation_name
);
