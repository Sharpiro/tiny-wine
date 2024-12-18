
#include "./pe_tools.h"
#include "../tiny_c/tiny_c.h"
#include "loader_lib.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

static const int MAX_ARRAY_LENGTH = 1000;

bool get_pe_data(int32_t fd, struct PeData *pe_data) {
    if (pe_data == NULL) {
        BAIL("pe_data was null\n");
    }

    int8_t *pe_header_buffer = loader_malloc_arena(1000);
    tiny_c_read(fd, pe_header_buffer, 1000);
    struct ImageDosHeader *dos_header =
        (struct ImageDosHeader *)pe_header_buffer;
    if (dos_header->magic != DOS_MAGIC) {
        BAIL("Invalid DOS header\n");
    }
    size_t image_header_start = (size_t)dos_header->e_lfanew;
    struct WinPEHeader *winpe_header =
        (struct WinPEHeader *)(pe_header_buffer + image_header_start);
    if (winpe_header->image_optional_header.magic != PE32_PLUS_MAGIC) {
        BAIL("Invalid PE header\n");
    }
    if (winpe_header->image_optional_header.number_of_rva_and_sizes != 16) {
        BAIL("unsupported data directory size");
    }

    off_t section_headers_start =
        (off_t)dos_header->e_lfanew + (off_t)sizeof(struct WinPEHeader);
    size_t section_headers_len =
        winpe_header->image_file_header.number_of_sections;
    size_t section_headers_size =
        sizeof(struct WinSectionHeader) * section_headers_len;
    struct WinSectionHeader *section_headers =
        loader_malloc_arena(section_headers_size);
    tinyc_lseek(fd, section_headers_start, SEEK_SET);
    tiny_c_read(fd, section_headers, section_headers_size);

    struct WinSectionHeader *idata_header = NULL;
    for (size_t i = 0; i < section_headers_len; i++) {
        if (tiny_c_strcmp((const char *)section_headers[i].name, ".idata") ==
            0) {
            idata_header = &section_headers[i];
        }
    }
    if (idata_header == NULL) {
        BAIL(".idata section not found");
    }

    uint8_t *idata_buffer = loader_malloc_arena(idata_header->size_of_raw_data);
    tinyc_lseek(fd, idata_header->pointer_to_raw_data, SEEK_SET);
    tiny_c_read(fd, idata_buffer, idata_header->size_of_raw_data);
    struct ImportDirectoryRawEntry *raw_dir_entries =
        (struct ImportDirectoryRawEntry *)idata_buffer;
    struct ImportDirectoryEntry *entries =
        (struct ImportDirectoryEntry *)loader_malloc_arena(
            sizeof(struct ImportDirectoryEntry) * MAX_ARRAY_LENGTH
        );

    size_t idata_base = idata_header->virtual_address;
    size_t import_dir_entries_len = 0;
    for (size_t i = 0; true; i++) {
        struct ImportDirectoryRawEntry *raw_dir_entry = &raw_dir_entries[i];
        if (i == MAX_ARRAY_LENGTH) {
            BAIL("unsupported .idata table size\n");
        }
        if (tiny_c_mem_is_empty(
                raw_dir_entry, IMPORT_DIRECTORY_RAW_ENTRY_SIZE
            )) {
            break;
        }

        import_dir_entries_len++;

        size_t name_idata_offset = raw_dir_entry->name_offset - idata_base;
        const char *lib_name = (char *)idata_buffer + name_idata_offset;

        struct ImportEntry *import_entries =
            loader_malloc_arena(sizeof(struct ImportEntry) * MAX_ARRAY_LENGTH);
        size_t import_entries_len = 0;
        uint64_t *import_lookup_entries =
            (uint64_t *)(idata_buffer +
                         (raw_dir_entry->characteristics - idata_base));
        uint64_t *import_address_entries =
            (uint64_t *)(idata_buffer +
                         (raw_dir_entry->import_address_table_offset -
                          idata_base));
        for (size_t j = 0; true; j++) {
            uint64_t import_lookup_entry = import_lookup_entries[j];
            uint64_t import_address_entry = import_address_entries[j];
            if (j == MAX_ARRAY_LENGTH) {
                BAIL("unsupported array size\n");
            }
            if (tiny_c_mem_is_empty(&import_lookup_entry, sizeof(uint64_t))) {
                break;
            }
            if (import_lookup_entry & 0x8000000000000000) {
                BAIL("unsupported import table lookup by ordinal\n");
            }

            import_entries_len++;

            const int NAME_ENTRY_OFFSET = 2;

            size_t import_name_entry_offset =
                (import_lookup_entry & 0x7fffffff) - idata_base;
            const char *import_name = (char *)idata_buffer +
                import_name_entry_offset + NAME_ENTRY_OFFSET;
            import_entries[j] = (struct ImportEntry){
                .name = import_name,
                .address = import_address_entry,
            };
        }

        entries[i] = (struct ImportDirectoryEntry){
            .import_lookup_table_offset = raw_dir_entry->characteristics,
            .lib_name = lib_name,
            .import_entries = import_entries,
            .import_entries_len = import_entries_len,
        };
    }

    size_t image_base = winpe_header->image_optional_header.image_base;
    size_t entrypoint =
        image_base + winpe_header->image_optional_header.address_of_entry_point;

    struct ImageDataDirectory *import_address_table_dir =
        &winpe_header->image_optional_header.data_directory[DATA_DIR_IAT_INDEX];
    size_t iat_len = import_address_table_dir->size == 0
        ? import_address_table_dir->size
        : import_address_table_dir->size / sizeof(size_t) - 1;

    size_t iat_file_offset =
        import_address_table_dir->virtual_address - idata_base;
    size_t import_address_table_offset =
        import_address_table_dir->virtual_address;

    struct KeyValue *import_address_table = NULL;
    if (iat_len > 0) {
        import_address_table =
            loader_malloc_arena(sizeof(struct KeyValue) * iat_len);
        size_t *iat_base = (size_t *)(idata_buffer + iat_file_offset);
        for (size_t i = 0; i < iat_len; i++) {
            size_t key = import_address_table_offset + i * sizeof(size_t);
            size_t value = *(iat_base + i);
            import_address_table[i] =
                (struct KeyValue){.key = key, .value = value};
        }
    }

    /* String table */
    size_t raw_symbols_len = winpe_header->image_file_header.number_of_symbols;
    size_t raw_symbols_size = sizeof(struct RawWinSymbol) * raw_symbols_len;
    size_t symbol_table_offset =
        winpe_header->image_file_header.pointer_to_symbol_table;
    size_t string_table_offset = symbol_table_offset + raw_symbols_size;
    tinyc_lseek(fd, (off_t)string_table_offset, SEEK_SET);
    size_t string_table_size = 0;
    tiny_c_read(fd, &string_table_size, 4);

    tinyc_lseek(fd, (off_t)string_table_offset, SEEK_SET);
    uint8_t *string_table = loader_malloc_arena(string_table_size);
    tiny_c_read(fd, string_table, string_table_size);

    /* Symbol table */
    struct RawWinSymbol *raw_symbols = loader_malloc_arena(raw_symbols_size);
    tinyc_lseek(fd, (off_t)symbol_table_offset, SEEK_SET);
    tiny_c_read(fd, raw_symbols, raw_symbols_size);

    struct WinSymbol *symbols =
        loader_malloc_arena(sizeof(struct WinSymbol) * raw_symbols_len);
    size_t symbols_len = 0;
    for (size_t i = 0; i < raw_symbols_len; i++) {
        struct RawWinSymbol *raw_symbol = &raw_symbols[i];
        char *name;
        if (tiny_c_mem_is_empty(raw_symbol->name, 4)) {
            uint32_t str_table_offset = *(uint32_t *)(raw_symbol->name + 4);
            name = (char *)string_table + str_table_offset;
        } else {
            name = loader_malloc_arena(sizeof(raw_symbol->name) + 1);
            memcpy(name, raw_symbol->name, sizeof(raw_symbol->name));
        }
        symbols[symbols_len++] = (struct WinSymbol){
            .name = name,
            .value = raw_symbol->value,
            .section_number = raw_symbol->section_number,
            .type = raw_symbol->type,
            .storage_class = raw_symbol->storage_class,
            .auxillary_symbols_len = raw_symbol->auxillary_symbols_len,
            .raw_index = i,
        };
        i += raw_symbol->auxillary_symbols_len;
    }

    *pe_data = (struct PeData){
        .dos_header = dos_header,
        .winpe_header = winpe_header,
        .entrypoint = entrypoint,
        .section_headers = section_headers,
        .section_headers_len = section_headers_len,
        .import_dir_entries = entries,
        .import_dir_entries_len = import_dir_entries_len,
        .import_address_table_offset = import_address_table_offset,
        .import_address_table = import_address_table,
        .import_address_table_len = iat_len,
        .symbols = symbols,
        .symbols_len = symbols_len,
    };

    return true;
}

bool get_memory_regions_info_win(
    const struct WinSectionHeader *program_headers,
    size_t program_headers_len,
    size_t address_offset,
    struct MemoryRegionsInfo *memory_regions_info
) {
    const int MAX_REGION_SIZE = 0x1000;

    struct MemoryRegion *memory_regions =
        loader_malloc_arena(sizeof(struct MemoryRegion) * program_headers_len);
    for (size_t i = 0; i < program_headers_len; i++) {
        const struct WinSectionHeader *program_header = &program_headers[i];
        if (program_header->virtual_size > MAX_REGION_SIZE) {
            BAIL("unsupported region size %x\n", program_header->virtual_size);
        }

        size_t win_permissions = program_header->characteristics & 0xff;
        size_t permissions;
        if (win_permissions == 0x20) {
            permissions = 4 | 0 | 1;
        } else if (win_permissions == 0x40) {
            permissions = 4 | 0 | 0;
        } else {
            BAIL("unsupported win permissions %x\n", win_permissions);
        }

        memory_regions[i] = (struct MemoryRegion){
            .start = address_offset + program_header->virtual_address,
            .end = address_offset + program_header->virtual_address +
                MAX_REGION_SIZE,
            .is_direct_file_map = false,
            .file_offset = program_header->pointer_to_raw_data,
            .file_size = program_header->virtual_size,
            .permissions = permissions,
        };
    }

    *memory_regions_info = (struct MemoryRegionsInfo){
        .start = 0,
        .end = 0,
        .regions = memory_regions,
        .regions_len = program_headers_len,
    };
    return true;
}

const struct WinSectionHeader *find_win_section_header(
    const struct WinSectionHeader *section_headers,
    size_t section_headers_len,
    const char *name
) {
    for (size_t i = 0; i < section_headers_len; i++) {
        const struct WinSectionHeader *section_header = &section_headers[i];
        if (tiny_c_strcmp((char *)section_header->name, name) == 0) {
            return section_header;
        }
    }

    return NULL;
}
