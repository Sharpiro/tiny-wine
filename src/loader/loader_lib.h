#pragma once

#include "elf_tools.h"
#include "list.h"
#include "pe_tools.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define LOADER_BUFFER_LEN 0x200'000
// @todo: hard-coding this may cause random program failures due to ASLR etc.
#define LOADER_SHARED_LIB_START 0x500000

extern int32_t loader_log_handle;

#ifdef VERBOSE

#define LOADER_LOG(fmt, ...)                                                   \
    tiny_c_fprintf(loader_log_handle, fmt, ##__VA_ARGS__);

#else

#define LOADER_LOG(fmt, ...)                                                   \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

typedef struct RuntimeGotEntry {
    size_t index;
    size_t value;
    size_t lib_dynamic_offset;
} RuntimeGotEntry;

CREATE_LIST_STRUCT(RuntimeGotEntry)

struct RuntimeRelocation {
    size_t offset;
    size_t value;
    const char *name;
    size_t type;
    size_t lib_dyn_offset;
};

typedef struct RuntimeSymbol {
    size_t value;
    const char *name;
    size_t size;
} RuntimeSymbol;

CREATE_LIST_STRUCT(RuntimeSymbol)

// @todo: make this generic?
typedef struct RuntimeObject {
    const char *name;
    size_t dynamic_offset;
    struct ElfData elf_data;
    struct MemoryRegionsInfo memory_regions_info;
    struct RuntimeRelocation *runtime_func_relocations;
    size_t runtime_func_relocations_len;
    uint8_t *bss;
    size_t bss_len;
    RuntimeSymbolList runtime_symbols;
} RuntimeObject;

typedef struct WinRuntimeExport {
    size_t address;
    const char *name;
} WinRuntimeExport;

CREATE_LIST_STRUCT(WinRuntimeExport)

typedef struct WinRuntimeObject {
    const char *name;
    struct PeData pe_data;
    struct MemoryRegionsInfo memory_regions_info;
    WinRuntimeExportList function_exports;
    size_t iat_runtime_base;
    size_t iat_runtime_offset;
} WinRuntimeObject;

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
    const struct RuntimeGotEntry *got_entries,
    size_t got_entries_len,
    size_t offset,
    struct RuntimeGotEntry **got_entry
);

void *loader_malloc_arena(size_t n);

void loader_free_arena(void);

bool read_to_string(const char *path, char **content, size_t size);

bool log_memory_regions(void);

bool get_function_relocations(
    const struct DynamicData *dyn_data,
    size_t dyn_offset,
    struct RuntimeRelocation **runtime_func_relocations,
    size_t *runtime_func_relocations_len
);

bool get_runtime_symbols(
    const struct DynamicData *dyn_data,
    size_t dyn_offset,
    RuntimeSymbolList *runtime_symbols
);

bool get_runtime_got(
    const struct DynamicData *dyn_data,
    size_t lib_dyn_offset,
    size_t dynamic_linker_callback_address,
    size_t *got_lib_dyn_offset_table,
    RuntimeGotEntryList *runtime_symbols
);
