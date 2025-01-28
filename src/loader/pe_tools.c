
#include "./pe_tools.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define MAX_ARRAY_LENGTH 1000

static bool find_import_entry(
    const struct ImportDirectoryEntry *dir_entries,
    size_t dir_entries_len,
    size_t iat_value,
    const char **lib_name,
    const struct ImportEntry **import_entry
) {
    for (size_t i = 0; i < dir_entries_len; i++) {
        const struct ImportDirectoryEntry *curr_dir_entry = &dir_entries[i];
        for (size_t j = 0; j < curr_dir_entry->import_entries_len; j++) {
            struct ImportEntry *curr_import_entry =
                &curr_dir_entry->import_entries[j];
            if (curr_import_entry->address == iat_value) {
                *import_entry = curr_import_entry;
                *lib_name = curr_dir_entry->lib_name;
                return true;
            }
        }
    }

    return false;
}

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
    bool is_64_bit =
        winpe_header->image_optional_header.magic == PE32_PLUS_MAGIC;
    if (!is_64_bit && winpe_header->image_optional_header.magic != PE32_MAGIC) {
        BAIL("Invalid PE header\n");
    }

    size_t data_dir_len = is_64_bit
        ? winpe_header->image_optional_header.data_directory_len
        : winpe_header->image_optional_header_32.data_directory_len;
    size_t image_base = is_64_bit
        ? winpe_header->image_optional_header.image_base
        : winpe_header->image_optional_header_32.image_base;
    size_t entrypoint =
        image_base + winpe_header->image_optional_header.address_of_entry_point;
    struct ImageDataDirectory *import_address_table_dir = is_64_bit
        ? &winpe_header->image_optional_header
               .data_directory[DATA_DIR_IAT_INDEX]
        : &winpe_header->image_optional_header_32
               .data_directory[DATA_DIR_IAT_INDEX];
    size_t section_headers_start = (size_t)dos_header->e_lfanew +
        WIN_OPTIONAL_HEADER_START +
        (is_64_bit ? sizeof(struct ImageOptionalHeader)
                   : sizeof(struct ImageOptionalHeader32));

    if (data_dir_len != 16) {
        BAIL("unsupported data directory size\n");
    }

    size_t section_headers_len =
        winpe_header->image_file_header.number_of_sections;
    size_t section_headers_size =
        sizeof(struct WinSectionHeader) * section_headers_len;
    struct WinSectionHeader *section_headers =
        loader_malloc_arena(section_headers_size);
    tinyc_lseek(fd, (off_t)section_headers_start, SEEK_SET);
    tiny_c_read(fd, section_headers, section_headers_size);

    /* Import data section */

    // @todo: import tables can be ANYWHERE LOL
    struct ImageDataDirectory import_dir =
        winpe_header->image_optional_header_32.data_directory[12];
    struct ImportDirectoryRawEntry *raw_dir_entries =
        loader_malloc_arena(import_dir.size);
    size_t raw_dir_entries_len =
        import_dir.size / sizeof(struct ImportDirectoryRawEntry);
    tinyc_lseek(fd, import_dir.virtual_address, SEEK_SET);
    tiny_c_read(fd, raw_dir_entries, import_dir.size);

    // const struct WinSectionHeader *idata_header =
    //     find_win_section_header(section_headers, section_headers_len,
    //     ".idata");
    // if (idata_header == NULL) {
    //     BAIL(".idata section not found\n");
    // }

    uint8_t *idata_buffer = NULL;
    // uint8_t *idata_buffer =
    // loader_malloc_arena(idata_header->size_of_raw_data); tinyc_lseek(fd,
    // idata_header->pointer_to_raw_data, SEEK_SET); tiny_c_read(fd,
    // idata_buffer, idata_header->size_of_raw_data); struct
    // ImportDirectoryRawEntry *raw_dir_entries =
    //     (struct ImportDirectoryRawEntry *)idata_buffer;
    struct ImportDirectoryEntry *import_dir_entries =
        (struct ImportDirectoryEntry *)loader_malloc_arena(
            sizeof(struct ImportDirectoryEntry) * MAX_ARRAY_LENGTH
        );

    // const size_t idata_base = idata_header->base_address;
    const size_t idata_base = 0;
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
            (uint64_t *)(idata_buffer + raw_dir_entry->characteristics -
                         idata_base);
        uint64_t *import_address_entries =
            (uint64_t *)(idata_buffer +
                         raw_dir_entry->import_address_table_offset -
                         idata_base);
        uint32_t *import_lookup_entries_32 = (uint32_t *)import_lookup_entries;
        uint32_t *import_address_entries_32 =
            (uint32_t *)(import_address_entries);
        for (size_t j = 0; true; j++) {
            size_t import_lookup_entry = is_64_bit
                ? import_lookup_entries[j]
                : import_lookup_entries_32[j];
            size_t import_address_entry = is_64_bit
                ? import_address_entries[j]
                : import_address_entries_32[j];
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

        import_dir_entries[i] = (struct ImportDirectoryEntry){
            .import_lookup_table_offset = raw_dir_entry->characteristics,
            .lib_name = lib_name,
            .import_entries = import_entries,
            .import_entries_len = import_entries_len,
        };
    }

    size_t iat_len = import_address_table_dir->size == 0
        ? import_address_table_dir->size
        : import_address_table_dir->size / sizeof(size_t) - 1;

    size_t iat_file_offset =
        import_address_table_dir->virtual_address - idata_base;
    size_t import_address_table_offset =
        import_address_table_dir->virtual_address;

    struct ImportAddressEntry *import_address_table = NULL;
    if (iat_len > 0) {
        import_address_table =
            loader_malloc_arena(sizeof(struct ImportAddressEntry) * iat_len);
        size_t *iat_base = (size_t *)(idata_buffer + iat_file_offset);
        for (size_t i = 0; i < iat_len; i++) {
            size_t key = import_address_table_offset + i * sizeof(size_t);
            size_t value = *(iat_base + i);
            const char *lib_name = NULL;
            const struct ImportEntry *import_entry;
            const char *import_name = NULL;
            if (find_import_entry(
                    import_dir_entries,
                    import_dir_entries_len,
                    value,
                    &lib_name,
                    &import_entry
                )) {
                import_name = import_entry->name;
            }
            import_address_table[i] = (struct ImportAddressEntry){
                .key = key,
                .value = value,
                .lib_name = lib_name,
                .import_name = import_name,
            };
        }
    }

    /* Export data section */

    const struct WinSectionHeader *edata_header =
        find_win_section_header(section_headers, section_headers_len, ".edata");
    struct ExportEntry *export_entries = NULL;
    size_t export_entries_len = 0;
    if (edata_header != NULL) {
        uint8_t *edata_buffer =
            loader_malloc_arena(edata_header->size_of_raw_data);
        tinyc_lseek(fd, edata_header->pointer_to_raw_data, SEEK_SET);
        tiny_c_read(fd, edata_buffer, edata_header->size_of_raw_data);
        struct ExportDirectoryRawEntry *export_directory =
            (struct ExportDirectoryRawEntry *)edata_buffer;
        if (export_directory->address_table_len !=
            export_directory->name_points_len) {
            BAIL("Unsupported address table data\n");
        }

        size_t edata_base = edata_header->base_address;
        uint32_t *address_offsets =
            (uint32_t *)(edata_buffer +
                         export_directory->export_address_table_offset -
                         edata_base);
        uint32_t *export_name_offsets =
            (uint32_t *)(edata_buffer + export_directory->name_pointer_offset -
                         edata_base);
        export_entries_len = export_directory->name_points_len;
        export_entries = loader_malloc_arena(
            sizeof(struct ExportEntry) * export_entries_len
        );
        for (size_t i = 0; i < export_entries_len; i++) {
            uint32_t address_offset = address_offsets[i];
            uint32_t name_offset = export_name_offsets[i];
            size_t name_file_offset = name_offset - edata_base;
            char *name = (char *)edata_buffer + name_file_offset;
            export_entries[i] = (struct ExportEntry){
                .address = address_offset,
                .name = name,
            };
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
        .import_dir_entries = import_dir_entries,
        .import_dir_entries_len = import_dir_entries_len,
        .import_address_table_offset = import_address_table_offset,
        .import_address_table = import_address_table,
        .import_address_table_len = iat_len,
        .export_entries = export_entries,
        .export_entries_len = export_entries_len,
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

        size_t win_permissions = program_header->characteristics >> 28;
        size_t read = win_permissions & 4;
        size_t write = win_permissions & 8 ? 2 : 0;
        size_t execute = win_permissions & 2 ? 1 : 0;
        size_t permissions = read | write | execute;

        memory_regions[i] = (struct MemoryRegion){
            .start = address_offset + program_header->base_address,
            .end =
                address_offset + program_header->base_address + MAX_REGION_SIZE,
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

bool map_memory_regions_win(
    int32_t fd, const struct MemoryRegion *regions, size_t regions_len
) {
    size_t regions_size = sizeof(struct MemoryRegion) * regions_len;
    struct MemoryRegion *editable_regions = loader_malloc_arena(regions_size);
    memcpy(editable_regions, regions, regions_size);

    for (size_t i = 0; i < regions_len; i++) {
        struct MemoryRegion *memory_region = &editable_regions[i];
        memory_region->permissions = 4 | 2 | 1;
    }

    if (!map_memory_regions(fd, editable_regions, regions_len)) {
        BAIL("loader map memory regions failed\n");
    }

    for (size_t i = 0; i < regions_len; i++) {
        const struct MemoryRegion *memory_region = &regions[i];
        uint8_t *region_start = (uint8_t *)memory_region->start;
        tinyc_lseek(fd, (off_t)memory_region->file_offset, SEEK_SET);
        if ((int32_t)tiny_c_read(fd, region_start, memory_region->file_size) <
            0) {
            BAIL("read failed\n");
        }

        int32_t prot_read = (memory_region->permissions & 4) >> 2;
        int32_t prot_write = memory_region->permissions & 2;
        int32_t prot_execute = ((int32_t)memory_region->permissions & 1) << 2;
        int32_t map_protection = prot_read | prot_write | prot_execute;
        size_t memory_region_len = memory_region->end - memory_region->start;
        if (tiny_c_mprotect(region_start, memory_region_len, map_protection) <
            0) {
            BAIL(
                "tiny_c_mprotect failed, %d, %s\n",
                tinyc_errno,
                tinyc_strerror(tinyc_errno)
            );
        }
    }

    return true;
}
