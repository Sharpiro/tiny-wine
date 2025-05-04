#pragma once

#include <loader/memory_map.h>
#include <loader/windows/pe_tools.h>
#include <stddef.h>

#define MAX_TRAMPOLINE_IAT_SIZE (512 * 8)

typedef union {
    uint8_t buffer[8];
    uint64_t u64;
} Converter;

struct PreservedSwapState {
    size_t rbx;
    size_t rdi;
    size_t rsi;
};

typedef struct WinRuntimeExport {
    size_t address;
    const char *name;
} WinRuntimeExport;

CREATE_LIST_STRUCT(WinRuntimeExport)

typedef struct WinRuntimeObject {
    const char *name;
    struct PeData pe_data;
    MemoryRegionList memory_regions;
    WinRuntimeExportList function_exports;
    // Runtime object's section base IAT trampoline region
    size_t runtime_iat_section_base;
} WinRuntimeObject;

CREATE_LIST_STRUCT(WinRuntimeObject)

extern inline Converter convert(size_t x);

const struct WinRuntimeObject *find_runtime_object(
    const WinRuntimeObjectList *runtime_objects, const char *name
);

const struct WinSymbol *find_win_symbol(
    const struct WinSymbol *symbols, size_t symbols_len, const char *name
);

typedef struct RuntimeImportAddressEntry {
    size_t key;
    size_t value;
    const char *lib_name;
    const char *import_name;
    bool is_variable;
} RuntimeImportAddressEntry;

CREATE_LIST_STRUCT(RuntimeImportAddressEntry)

bool get_runtime_import_address_table(
    const struct ImportAddressEntry *import_address_table,
    size_t import_address_table_len,
    const WinRuntimeObjectList *shared_libraries,
    RuntimeImportAddressEntryList *runtime_import_table,
    size_t image_base,
    size_t runtime_iat_base
);

bool map_import_address_table(
    const RuntimeImportAddressEntryList *import_address_table,
    size_t dynamic_callback_windows,
    size_t iat_runtime_base
);

bool get_memory_regions_win(
    const struct WinSectionHeader *program_headers,
    size_t program_headers_len,
    size_t address_offset,
    MemoryRegionList *memory_regions
);

bool map_memory_regions_win(
    int32_t fd, const struct MemoryRegion *regions, size_t regions_len
);
