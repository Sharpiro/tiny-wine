#include <dlls/twlibc.h>
#include <loader/windows/pe_tools.h>
#include <stddef.h>
#include <stdint.h>

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

static bool mem_is_empty(const void *buffer, size_t n) {
    for (size_t i = 0; i < n; i++) {
        uint8_t byte = ((uint8_t *)buffer)[i];
        if (byte != 0) {
            return false;
        }
    }

    return true;
}

bool get_pe_data(FILE *fp, struct PeData *pe_data) {
    if (pe_data == NULL) {
        BAIL("pe_data was null\n");
    }

    uint8_t *pe_header_buffer = malloc(1000);
    if (!pe_header_buffer) {
        BAIL("malloc failed\n");
    }

    const size_t READ_LEN = 1'000;
    size_t read_result = fread(pe_header_buffer, 1, READ_LEN, fp);
    if (read_result != READ_LEN) {
        BAIL("failed to read enough data\n");
    }

    struct ImageDosHeader *dos_header =
        (struct ImageDosHeader *)pe_header_buffer;
    if (dos_header->magic != DOS_MAGIC) {
        BAIL("Invalid DOS header\n");
    }

    size_t image_header_start = (size_t)dos_header->image_file_header_start;
    struct WinPEHeader *winpe_header =
        (struct WinPEHeader *)(pe_header_buffer + image_header_start);
    if (winpe_header->image_optional_header_64.magic != PE32_PLUS_MAGIC &&
        winpe_header->image_optional_header_32.magic != PE32_MAGIC) {
        BAIL("Invalid PE header magic\n");
    }

    bool is_64_bit =
        winpe_header->image_optional_header_64.magic == PE32_PLUS_MAGIC;
    size_t pe_word_size = is_64_bit ? 8 : 4;
    struct ImageOptionalHeader image_optional_header;
    if (is_64_bit) {
        struct ImageOptionalHeader64 *optional_header =
            &winpe_header->image_optional_header_64;
        image_optional_header = (struct ImageOptionalHeader){
            .magic = optional_header->magic,
            .address_of_entry_point = optional_header->address_of_entry_point,
            .base_of_code = optional_header->base_of_code,
            .image_base = optional_header->image_base,
            .data_directory_len = optional_header->data_directory_len,
            .data_directory = {},
            .image_optional_header_size = sizeof(*optional_header),
            .image_size = optional_header->size_of_image,
            .headers_size = optional_header->size_of_headers,
        };
        memcpy(
            image_optional_header.data_directory,
            optional_header->data_directory,
            sizeof(struct ImageDataDirectory) *
                optional_header->data_directory_len
        );
    } else {
        struct ImageOptionalHeader32 *optional_header =
            &winpe_header->image_optional_header_32;
        image_optional_header = (struct ImageOptionalHeader){
            .magic = optional_header->magic,
            .address_of_entry_point = optional_header->address_of_entry_point,
            .base_of_code = optional_header->base_of_code,
            .image_base = optional_header->image_base,
            .data_directory_len = optional_header->data_directory_len,
            .data_directory = {},
            .image_optional_header_size = sizeof(*optional_header),
            .image_size = optional_header->size_of_image,
            .headers_size = optional_header->size_of_headers,
        };
        memcpy(
            image_optional_header.data_directory,
            optional_header->data_directory,
            sizeof(struct ImageDataDirectory) *
                optional_header->data_directory_len
        );
    }

    size_t image_base = image_optional_header.image_base;
    size_t entrypoint =
        image_base + image_optional_header.address_of_entry_point;
    struct ImageDataDirectory *import_address_table_dir =
        &image_optional_header.data_directory[DATA_DIR_IAT_INDEX];
    size_t section_headers_start = (size_t)dos_header->image_file_header_start +
        WIN_OPTIONAL_HEADER_START +
        image_optional_header.image_optional_header_size;

    if (image_optional_header.data_directory_len != 16) {
        BAIL("unsupported data directory size\n");
    }

    size_t section_headers_len =
        winpe_header->image_file_header.number_of_sections;
    size_t section_headers_size =
        sizeof(struct WinSectionHeader) * section_headers_len;
    struct WinSectionHeader *section_headers = malloc(section_headers_size);
    fseek(fp, (off_t)section_headers_start, SEEK_SET);
    fread(section_headers, 1, section_headers_size, fp);

    /* Import Directory */

    struct ImageDataDirectory *import_dir =
        &image_optional_header.data_directory[DATA_DIR_IMPORT_DIR_INDEX];
    struct WinSectionHeader *import_section = NULL;
    for (ssize_t i = (ssize_t)section_headers_len - 1; i >= 0; i--) {
        struct WinSectionHeader *curr_section = &section_headers[i];
        if (import_dir->virtual_address >= curr_section->virtual_base_address) {
            import_section = curr_section;
            break;
        }
    }

    struct ImportDirectoryEntry *import_dir_entries = NULL;
    size_t import_dir_entries_len = 0;
    struct ImportAddressEntry *import_address_table = NULL;
    size_t import_address_table_len = 0;
    if (import_section != NULL) {
        uint8_t *import_section_buffer = malloc(import_section->file_size);

        fseek(fp, import_section->file_offset, SEEK_SET);
        fread(import_section_buffer, 1, import_section->file_size, fp);

        size_t import_dir_section_offset =
            import_dir->virtual_address - import_section->virtual_base_address;
        struct ImportDirectoryRawEntry *raw_dir_entries =
            (void *)(import_section_buffer + import_dir_section_offset);
        import_dir_entries =
            malloc(sizeof(struct ImportDirectoryEntry) * MAX_ARRAY_LENGTH);

        const size_t import_dir_base = import_section->virtual_base_address;
        for (size_t i = 0; true; i++) {
            struct ImportDirectoryRawEntry *raw_dir_entry = &raw_dir_entries[i];
            if (i == MAX_ARRAY_LENGTH) {
                BAIL("unsupported .idata table size\n");
            }
            if (mem_is_empty(raw_dir_entry, IMPORT_DIRECTORY_RAW_ENTRY_SIZE)) {
                break;
            }

            import_dir_entries_len++;

            size_t name_idata_offset =
                raw_dir_entry->name_offset - import_dir_base;
            const char *lib_name =
                (char *)import_section_buffer + name_idata_offset;

            struct ImportEntry *import_entries =
                malloc(sizeof(struct ImportEntry) * MAX_ARRAY_LENGTH);
            size_t import_entries_len = 0;
            uint8_t *import_lookup_entries = import_section_buffer +
                raw_dir_entry->import_lookup_table_offset - import_dir_base;
            uint8_t *import_address_entries = import_section_buffer +
                raw_dir_entry->import_address_table_offset - import_dir_base;
            for (size_t j = 0; true; j++) {
                if (j == MAX_ARRAY_LENGTH) {
                    BAIL("unsupported array size\n");
                }

                size_t import_lookup_entry = 0;
                size_t import_address_entry = 0;
                memcpy(
                    &import_lookup_entry,
                    import_lookup_entries + j * pe_word_size,
                    pe_word_size
                );
                memcpy(
                    &import_address_entry,
                    import_address_entries + j * pe_word_size,
                    pe_word_size
                );

                if (mem_is_empty(&import_lookup_entry, pe_word_size)) {
                    break;
                }

                import_entries_len++;

                const char *import_name = NULL;
                size_t ordinal_mask = (uint32_t)0x8 << (pe_word_size * 8 - 4);
                if (import_lookup_entry & ordinal_mask) {
                    import_name = "<ordinal>";
                } else {
                    const int NAME_ENTRY_OFFSET = 2;
                    size_t lookup_value =
                        import_lookup_entry & ordinal_mask - 1;
                    size_t import_name_entry_offset =
                        lookup_value - import_dir_base;
                    import_name = (char *)import_section_buffer +
                        import_name_entry_offset + NAME_ENTRY_OFFSET;
                }

                import_entries[j] = (struct ImportEntry){
                    .name = import_name,
                    .address = import_address_entry,
                };
            }

            import_dir_entries[i] = (struct ImportDirectoryEntry){
                .import_lookup_table_offset =
                    raw_dir_entry->import_lookup_table_offset,
                .lib_name = lib_name,
                .import_entries = import_entries,
                .import_entries_len = import_entries_len,
            };
        }

        /** Import Address Table (IAT) */

        import_address_table_len = import_address_table_dir->size == 0
            ? import_address_table_dir->size
            : import_address_table_dir->size / pe_word_size - 1;

        size_t iat_file_offset =
            import_address_table_dir->virtual_address - import_dir_base;
        size_t import_address_table_offset =
            import_address_table_dir->virtual_address;

        if (import_address_table_len > 0) {
            import_address_table = malloc(
                sizeof(struct ImportAddressEntry) * import_address_table_len
            );
            uint8_t *iat_base = (import_section_buffer + iat_file_offset);
            for (size_t i = 0; i < import_address_table_len; i++) {
                size_t key = import_address_table_offset + i * pe_word_size;
                size_t value;
                memcpy(&value, iat_base + i * pe_word_size, pe_word_size);

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
    }

    /* Export data section */

    struct ImageDataDirectory *export_dir =
        &image_optional_header.data_directory[DATA_DIR_EXPORT_DIR_INDEX];
    struct WinSectionHeader *export_section = NULL;
    for (ssize_t i = (ssize_t)section_headers_len - 1; i >= 0; i--) {
        struct WinSectionHeader *curr_section = &section_headers[i];
        if (export_dir->virtual_address >= curr_section->virtual_base_address) {
            export_section = curr_section;
            break;
        }
    }

    const char *export_section_name = NULL;
    struct ExportEntry *export_entries = NULL;
    size_t export_entries_len = 0;
    if (export_section != NULL) {
        export_section_name = (const char *)export_section->name;
        uint8_t *export_section_buffer = malloc(export_section->file_size);
        if (!export_section_buffer) {
            BAIL("export_section_buffer malloc failed\n");
        }
        fseek(fp, export_section->file_offset, SEEK_SET);
        fread(export_section_buffer, 1, export_section->file_size, fp);

        size_t export_dir_section_offset =
            export_dir->virtual_address - export_section->virtual_base_address;
        struct ExportDirectoryRawEntry *export_dir_entry =
            (void *)(export_section_buffer + export_dir_section_offset);
        if (export_dir_entry->address_table_len <
            export_dir_entry->name_points_len) {
            BAIL("Unsupported address table data\n");
        }

        size_t export_section_base = export_section->virtual_base_address;
        uint32_t *address_offsets =
            (uint32_t *)(export_section_buffer +
                         export_dir_entry->export_address_table_offset -
                         export_section_base);
        uint32_t *export_name_offsets =
            (uint32_t *)(export_section_buffer +
                         export_dir_entry->name_pointer_offset -
                         export_section_base);
        export_entries_len = export_dir_entry->address_table_len;
        export_entries =
            malloc(sizeof(struct ExportEntry) * export_entries_len);
        if (!export_entries) {
            BAIL("export_entries malloc failed\n");
        }
        for (size_t i = 0; i < export_entries_len; i++) {
            uint32_t address_offset = address_offsets[i];
            if (i >= export_dir_entry->name_points_len) {
                export_entries[i] = (struct ExportEntry){
                    .address = address_offset,
                    .name = "<unknown>",
                };
                continue;
            }

            uint32_t name_offset = export_name_offsets[i];
            size_t name_file_offset = name_offset - export_section_base;
            char *name = (char *)export_section_buffer + name_file_offset;
            export_entries[i] = (struct ExportEntry){
                .address = address_offset,
                .name = name,
            };
        }
    }

    /* Symbol table */

    size_t raw_symbols_len = winpe_header->image_file_header.number_of_symbols;
    struct WinSymbol *symbols = NULL;
    size_t symbols_len = 0;
    if (raw_symbols_len) {
        size_t raw_symbols_size = sizeof(struct RawWinSymbol) * raw_symbols_len;
        size_t symbol_table_offset =
            winpe_header->image_file_header.pointer_to_symbol_table;
        size_t string_table_offset = symbol_table_offset + raw_symbols_size;
        fseek(fp, (off_t)string_table_offset, SEEK_SET);
        size_t string_table_size = 0;
        fread(&string_table_size, 1, 4, fp);

        fseek(fp, (off_t)string_table_offset, SEEK_SET);
        uint8_t *string_table = malloc(string_table_size);
        fread(string_table, 1, string_table_size, fp);

        struct RawWinSymbol *raw_symbols = malloc(raw_symbols_size);
        fseek(fp, (off_t)symbol_table_offset, SEEK_SET);
        fread(raw_symbols, 1, raw_symbols_size, fp);

        symbols = malloc(sizeof(struct WinSymbol) * raw_symbols_len);
        for (size_t i = 0; i < raw_symbols_len; i++) {
            struct RawWinSymbol *raw_symbol = &raw_symbols[i];
            char *name;
            if (mem_is_empty(raw_symbol->name, 4)) {
                uint32_t str_table_offset = *(uint32_t *)(raw_symbol->name + 4);
                name = (char *)string_table + str_table_offset;
            } else {
                name = malloc(sizeof(raw_symbol->name) + 1);
                memcpy(name, raw_symbol->name, sizeof(raw_symbol->name));
            }
            symbols[symbols_len++] = (struct WinSymbol){
                .name = name,
                .value = raw_symbol->value,
                .section_index = raw_symbol->section_number - 1,
                .type = raw_symbol->type,
                .storage_class = raw_symbol->storage_class,
                .auxillary_symbols_len = raw_symbol->auxillary_symbols_len,
                .raw_index = i,
            };
            i += raw_symbol->auxillary_symbols_len;
        }
    }

    /* Relocations */

    struct ImageDataDirectory *relocation_dir =
        &image_optional_header.data_directory[DATA_DIR_RELOC_INDEX];
    RelocationEntryList relocations = {
        .allocator = malloc,
    };
    const struct WinSectionHeader *relocation_section =
        find_win_section_header(section_headers, section_headers_len, ".reloc");
    if (relocation_section) {
        uint8_t *relocation_section_buffer =
            malloc(relocation_section->file_size);
        fseek(fp, (off_t)relocation_section->file_offset, SEEK_SET);
        fread(relocation_section_buffer, 1, relocation_section->file_size, fp);

        const size_t RELOCATION_BLOCK_SIZE = sizeof(struct RelocationBlock);
        size_t block_bytes_parsed = 0;
        while (block_bytes_parsed < relocation_dir->size) {
            struct RelocationBlock *relocation_block =
                (struct RelocationBlock *)relocation_section_buffer;
            size_t relocations_len =
                (relocation_block->block_size - RELOCATION_BLOCK_SIZE) /
                sizeof(uint16_t);
            uint16_t *raw_relocations =
                (uint16_t *)(relocation_section_buffer + RELOCATION_BLOCK_SIZE);
            for (size_t i = 0; i < relocations_len; i++) {
                uint16_t raw_relocation = raw_relocations[i];
                size_t type = raw_relocation >> 12;
                size_t offset = raw_relocation & 0x0fff;
                struct RelocationEntry relocation_entry = {
                    .offset = offset,
                    .type = type,
                    .block_page_rva = relocation_block->page_rva,
                };
                bool add_result =
                    RelocationEntryList_add(&relocations, relocation_entry);
                if (!add_result) {
                    BAIL("Failed adding relocation to list\n");
                }
            }
            block_bytes_parsed += relocation_block->block_size;
            relocation_section_buffer += relocation_block->block_size;
        }
    }

    *pe_data = (struct PeData){
        .dos_header = dos_header,
        .winpe_header = winpe_header,
        .winpe_optional_header = image_optional_header,
        .entrypoint = entrypoint,
        .section_headers = section_headers,
        .section_headers_len = section_headers_len,
        .import_section = import_section,
        .import_dir_entries = import_dir_entries,
        .import_dir_entries_len = import_dir_entries_len,
        .import_address_table = import_address_table,
        .import_address_table_len = import_address_table_len,
        .export_section_name = export_section_name,
        .export_entries = export_entries,
        .export_entries_len = export_entries_len,
        .symbols = symbols,
        .symbols_len = symbols_len,
        .relocations = relocations,
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
        if (strcmp((char *)section_header->name, name) == 0) {
            return section_header;
        }
    }

    return NULL;
}
