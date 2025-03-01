#pragma once

#include "loader_lib.h"
#include "pe_tools.h"
#include <stddef.h>

// @todo: increment so large it breaks when you have more than 2 dlls
//        should probably dynamically get real IAT lengths
// @todo: doesn't work > 0x1000
#define IAT_INCREMENT 0x10000
#define IAT_LENGTH 0x1000

typedef union {
    uint8_t buffer[8];
    uint64_t u64;
} Converter;

extern inline Converter convert(size_t x);

const struct WinRuntimeObject *find_runtime_object(
    const WinRuntimeObjectList *runtime_objects, const char *name
);

const struct WinSymbol *find_win_symbol(
    const struct WinSymbol *symbols, size_t symbols_len, const char *name
);

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
