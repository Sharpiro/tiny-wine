#pragma once

#include "elf_tools.h"
#include "memory_map.h"

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

typedef struct RuntimeObject {
    const char *name;
    size_t dynamic_offset;
    struct ElfData elf_data;
    MemoryRegionList memory_regions;
    struct RuntimeRelocation *runtime_func_relocations;
    size_t runtime_func_relocations_len;
    uint8_t *bss;
    size_t bss_len;
    RuntimeSymbolList runtime_symbols;
} RuntimeObject;

bool find_runtime_relocation(
    const struct RuntimeRelocation *runtime_relocations,
    size_t runtime_relocations_len,
    size_t relocation_offset,
    const struct RuntimeRelocation **runtime_relocation
);

typedef struct RuntimeImportAddressEntry {
    size_t key;
    size_t value;
    const char *lib_name;
    const char *import_name;
    bool is_variable;
} RuntimeImportAddressEntry;

CREATE_LIST_STRUCT(RuntimeImportAddressEntry)

bool find_runtime_symbol(
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

bool read_to_string(const char *path, char **content, size_t size);

bool log_memory_regions(void);

bool get_function_relocations(
    const struct DynamicData *dyn_data,
    size_t dyn_offset,
    struct RuntimeRelocation **runtime_func_relocations,
    size_t *runtime_func_relocations_len
);

bool find_symbols(
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

bool get_memory_regions(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    MemoryRegionList *memory_regions
);
