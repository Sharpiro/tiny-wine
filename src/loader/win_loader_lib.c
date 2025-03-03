#include "./win_loader_lib.h"
#include "../tiny_c/tiny_c.h"
#include "loader_lib.h"
#include "memory_map.h"
#include "pe_tools.h"
#include <stddef.h>

#define ASM_X64_MOV32_IMMEDIATE 0xb8
#define ASM_X64_CALL 0xff, 0xd0

inline Converter convert(size_t x) {
    return (Converter){.u64 = x};
}

const struct WinRuntimeObject *find_runtime_object(
    const WinRuntimeObjectList *runtime_objects, const char *name
) {
    for (size_t i = 0; i < runtime_objects->length; i++) {
        const struct WinRuntimeObject *curr_obj = &runtime_objects->data[i];
        if (tiny_c_strcmp(curr_obj->name, name) == 0) {
            return curr_obj;
        }
    }
    return NULL;
}

const struct WinSymbol *find_win_symbol(
    const struct WinSymbol *symbols, size_t symbols_len, const char *name
) {
    for (size_t i = 0; i < symbols_len; i++) {
        const struct WinSymbol *curr_symbol = &symbols[i];
        if (tiny_c_strcmp(curr_symbol->name, name) == 0) {
            return curr_symbol;
        }
    }
    return NULL;
}

bool get_runtime_import_address_table(
    const struct ImportAddressEntry *import_address_table,
    size_t import_address_table_len,
    const WinRuntimeObjectList *shared_libraries,
    RuntimeImportAddressEntryList *runtime_import_table,
    size_t iat_image_base,
    size_t runtime_iat_region_base
) {
    for (size_t i = 0; i < import_address_table_len; i++) {
        const struct ImportAddressEntry *current_import =
            &import_address_table[i];
        size_t runtime_import_key = iat_image_base + current_import->key;
        if (current_import->value == 0) {
            struct RuntimeImportAddressEntry runtime_import = {
                .key = runtime_import_key,
                .value = 0,
                .lib_name = current_import->lib_name,
                .import_name = current_import->import_name,
                .is_variable = false,
            };
            RuntimeImportAddressEntryList_add(
                runtime_import_table, runtime_import
            );
            continue;
        }

        const struct WinSymbol *symbol = NULL;
        size_t import_image_base = 0;
        const struct WinRuntimeObject *import_runtime_obj =
            find_runtime_object(shared_libraries, current_import->lib_name);
        if (import_runtime_obj == NULL &&
            tiny_c_strcmp(current_import->lib_name, "ntdll.dll") != 0) {
            BAIL("expected runtime_obj '%s'\n", current_import->lib_name);
        }

        if (import_runtime_obj != NULL) {
            import_image_base = import_runtime_obj->pe_data.winpe_header
                                    ->image_optional_header.image_base;
            symbol = find_win_symbol(
                import_runtime_obj->pe_data.symbols,
                import_runtime_obj->pe_data.symbols_len,
                current_import->import_name
            );
        }

        size_t runtime_import_value =
            runtime_iat_region_base + current_import->value;
        bool is_variable = false;
        if (symbol != NULL) {
            is_variable = symbol->type != SYMBOL_TYPE_FUNCTION &&
                symbol->storage_class == SYMBOL_CLASS_EXTERNAL;
            if (is_variable) {
                struct WinSectionHeader *variable_section_header =
                    &import_runtime_obj->pe_data
                         .section_headers[symbol->section_index];
                if (symbol->value < 0) {
                    BAIL(
                        "unexpected negative symbol value for %s\n",
                        symbol->name
                    );
                }
                runtime_import_value = import_image_base +
                    variable_section_header->virtual_base_address +
                    (size_t)symbol->value;
            }
        }

        struct RuntimeImportAddressEntry runtime_import = {
            .key = runtime_import_key,
            .value = runtime_import_value,
            .lib_name = current_import->lib_name,
            .import_name = current_import->import_name,
            .is_variable = is_variable,
        };
        RuntimeImportAddressEntryList_add(runtime_import_table, runtime_import);
    }

    return true;
}

bool map_import_address_table(
    const RuntimeImportAddressEntryList *import_address_table,
    size_t dynamic_callback_windows,
    size_t runtime_iat_section_base
) {
    if (dynamic_callback_windows > UINT32_MAX) {
        BAIL("dynamic_callback_windows location exceeds 32 bits\n");
    }
    if (!import_address_table->length) {
        return true;
    }

    if (import_address_table->length * 8 > MAX_TRAMPOLINE_IAT_SIZE) {
        BAIL("import_address_table length must be <= 512\n");
    }
    struct MemoryRegion iat_region = {
        .start = runtime_iat_section_base,
        .end = runtime_iat_section_base + MAX_TRAMPOLINE_IAT_SIZE,
        .is_direct_file_map = false,
        .file_offset = 0,
        .file_size = 0,
        .permissions = 4 | 2 | 1,
    };
    if (!map_memory_regions(0, &iat_region, 1)) {
        BAIL("map_memory_regions failed\n");
    }

    for (size_t i = 0; i < import_address_table->length; i++) {
        const struct RuntimeImportAddressEntry *current_import =
            &import_address_table->data[i];
        if (current_import->value == 0) {
            LOADER_LOG("WARNING: IAT %x is 0\n", current_import->key);
            continue;
        }

        size_t *runtime_import_key = (size_t *)current_import->key;
        *runtime_import_key = current_import->value;

        LOADER_LOG(
            "IAT: %x, %x, %s\n",
            current_import->key,
            current_import->value,
            current_import->is_variable ? "variable" : "function"
        );

        if (current_import->is_variable) {
            continue;
        }

        Converter dyn_callback_converter = convert(dynamic_callback_windows);
        uint8_t trampoline_code[DYNAMIC_CALLBACK_TRAMPOLINE_SIZE] = {
            ASM_X64_MOV32_IMMEDIATE,
            dyn_callback_converter.buffer[0],
            dyn_callback_converter.buffer[1],
            dyn_callback_converter.buffer[2],
            dyn_callback_converter.buffer[3],
            ASM_X64_CALL,
        };
        memcpy(
            (uint8_t *)(current_import->value),
            trampoline_code,
            sizeof(trampoline_code)
        );
    }

    return true;
}
