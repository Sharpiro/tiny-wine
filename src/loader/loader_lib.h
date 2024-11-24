#pragma once

#include "elf_tools.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define LOADER_BUFFER_ADDRESS 0x7d8d0000
#define LOADER_BUFFER_LEN 0x10'000
#define LOADER_SHARED_LIB_START 0x500000

extern int32_t loader_log_handle;

void *loader_malloc_arena(size_t n);

void loader_free_arena(void);

bool read_to_string(const char *path, char **content, size_t size);

bool print_memory_regions(void);

#ifdef VERBOSE

#define LOADER_LOG(fmt, ...)                                                   \
    tiny_c_fprintf(loader_log_handle, fmt, ##__VA_ARGS__);

#else

#define LOADER_LOG(fmt, ...)                                                   \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

struct RuntimeRelocation {
    size_t offset;
    size_t value;
    const char *name;
    size_t type;
    size_t lib_dyn_offset;
};

struct SharedLibrary {
    const char *name;
    size_t dynamic_offset;
    struct ElfData elf_data;
    struct MemoryRegionsInfo memory_regions_info;
    struct RuntimeRelocation *runtime_func_relocations;
    size_t runtime_func_relocations_len;
    uint8_t *bss;
    size_t bss_len;
};

struct RuntimeSymbol {
    size_t value;
    const char *name;
    size_t size;
};

bool find_runtime_relocation(
    const struct RuntimeRelocation *runtime_relocations,
    size_t runtime_relocations_len,
    size_t relocation_offset,
    const struct RuntimeRelocation **runtime_relocation
);

bool get_runtime_symbol(
    const char *name,
    const struct RuntimeSymbol *runtime_symbols,
    size_t runtime_symbols_len,
    size_t ignore_val,
    const struct RuntimeSymbol **symbol
);

bool find_got_entry(
    const struct GotEntry *got_entries,
    size_t got_entries_len,
    size_t offset,
    struct GotEntry **got_entry
);
