#include "./win_loader_lib.h"
#include "../tiny_c/tiny_c.h"
#include "memory_map.h"
#include "pe_tools.h"
#include <stddef.h>

#define ASM_X64_MOV32_IMMEDIATE 0xb8
#define ASM_X64_CALL 0xff, 0xd0

typedef union {
    uint8_t buffer[8];
    uint64_t u64;
} Converter;

Converter convert(size_t x) {
    return (Converter){.u64 = x};
}

bool get_runtime_import_address_table(
    const struct ImportAddressEntry *import_address_table,
    size_t import_address_table_len,
    const WinRuntimeObjectList *shared_libraries,
    RuntimeImportAddressEntryList *runtime_import_table,
    size_t iat_image_base,
    size_t runtime_iat_base,
    struct WinSectionHeader *section_headers
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
        size_t runtime_obj_image_base = 0;
        if (shared_libraries != NULL && current_import->lib_name != NULL) {
            const struct WinRuntimeObject *runtime_obj = find_runtime_object(
                shared_libraries->data,
                shared_libraries->length,
                current_import->lib_name
            );
            if (runtime_obj != NULL) {
                // BAIL("expected runtime object '%s'\n",
                // current_import->lib_name);
                symbol = find_runtime_symbol(
                    runtime_obj->pe_data.symbols,
                    runtime_obj->pe_data.symbols_len,
                    current_import->import_name
                );
                runtime_obj_image_base = runtime_obj->pe_data.winpe_header
                                             ->image_optional_header.image_base;
                // if (symbol == NULL) {
                //     BAIL("expected symbol '%s'\n",
                //     current_import->import_name);
                // }
            }
        }

        // if (symbol->type == SYMBOL_TYPE_FUNCTION) {
        //     BAIL("unimplemented");
        // }
        // if (symbol->storage_class != SYMBOL_CLASS_EXTERNAL) {
        //     BAIL("unsupported storage class '%x'", symbol->storage_class);
        // }

        // size_t iat_runtime_base = runtime_iat_base + idata_base;
        size_t runtime_import_value =
            (runtime_iat_base + current_import->value);
        bool is_variable = false;
        size_t symbol_section = 0;
        ssize_t section_offset = 0;
        if (symbol != NULL) {
            is_variable = symbol->type != SYMBOL_TYPE_FUNCTION &&
                symbol->storage_class == SYMBOL_CLASS_EXTERNAL;
            symbol_section = symbol->section_number;
            section_offset = symbol->value;

            if (is_variable) {
                size_t one_based_index = symbol->section_number - 1;
                struct WinSectionHeader *variable_section_header =
                    &section_headers[one_based_index];
                if (symbol->value < 0) {
                    BAIL(
                        "unexpected negative symbol value for %s\n",
                        symbol->name
                    );
                }
                runtime_import_value = runtime_obj_image_base +
                    variable_section_header->base_address +
                    (size_t)symbol->value;
            }
        }

        // @todo should maybe always have valid symbol
        struct RuntimeImportAddressEntry runtime_import = {
            .key = runtime_import_key,
            .value = runtime_import_value,
            .lib_name = current_import->lib_name,
            .import_name = current_import->import_name,
            .is_variable = is_variable,
            .symbol_section = symbol_section,
            .section_offset = section_offset,
            // .is_variable = is_variable,
            // .symbol_section = symbol->section_number,
            // .section_offset = (ssize_t)symbol->value,
        };
        RuntimeImportAddressEntryList_add(runtime_import_table, runtime_import);
    }

    return true;
}

bool map_import_address_table(
    int32_t fd,
    size_t runtime_iat_base,
    size_t idata_base,
    const RuntimeImportAddressEntryList *import_address_table,
    size_t dynamic_callback_windows,
    size_t *iat_runtime_base
) {
    if (dynamic_callback_windows > UINT32_MAX) {
        BAIL("dynamic_callback_windows location exceeds 32 bits");
    }

    *iat_runtime_base = runtime_iat_base + idata_base;
    struct MemoryRegion iat_region = {
        .start = *iat_runtime_base,
        .end = runtime_iat_base + idata_base + 0x1000,
        .is_direct_file_map = false,
        .file_offset = 0,
        .file_size = 0,
        .permissions = 4 | 2 | 1,
    };
    // @todo: maybe memory only needs to be mapped if table is not all variables
    if (!map_memory_regions(fd, &iat_region, 1)) {
        EXIT("loader map memory regions failed\n");
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

        if (current_import->is_variable) {
            continue;
        }

        // @todo: comp-time?
        Converter dyn_callback_converter = convert(dynamic_callback_windows);
        const uint8_t trampoline_code[DYNAMIC_CALLBACK_TRAMPOLINE_SIZE] = {
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
        LOADER_LOG("IAT: %x, %x\n", current_import->key, current_import->value);
    }

    return true;
}
