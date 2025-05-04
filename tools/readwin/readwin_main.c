#include <dlls/twlibc.h>
#include <loader/windows/pe_tools.h>
#include <stddef.h>
#include <sys_linux.h>

// @todo: get working via winloader?

int main(int argc, char **argv) {
    if (argc < 2 || memcmp(argv[1], "--", 2) == 0) {
        EXIT("Usage: readwin <file> [-s]\n");
    }

    char *filename = argv[1];
    int32_t fd = open(filename, O_RDONLY);
    if (fd < 0) {
        EXIT("file not found\n");
    }

    const bool show_symbols = argc >= 3 && strcmp(argv[2], "-s") == 0;

    struct PeData pe_data;
    if (!get_pe_data(fd, &pe_data)) {
        EXIT("failed getting PE data\n");
    }
    close(fd);

    size_t dos_magic = pe_data.dos_header->magic;
    size_t pe_magic = pe_data.winpe_optional_header.magic;
    char *class = pe_magic == PE32_MAGIC ? "PE32" : "PE32+";
    size_t section_headers_start =
        (size_t)pe_data.dos_header->image_file_header_start +
        sizeof(struct WinPEHeader);
    size_t base_of_code = pe_data.winpe_optional_header.base_of_code;
    size_t image_base = pe_data.winpe_optional_header.image_base;

    /* General information */

    printf("File: %s\n\n", filename);
    printf("PE Header:\n");
    printf("DOS magic: %s\n", (char *)&dos_magic);
    printf("PE magic: 0x%zx\n", pe_magic);
    printf("Class: %s\n", class);
    printf("Base of code: 0x%zx\n", base_of_code);
    printf("Image base: 0x%zx\n", image_base);
    printf("Entry: 0x%zx\n", pe_data.entrypoint);
    printf("Section headers start: 0x%zx\n", section_headers_start);
    printf("Section headers length: %zd\n", pe_data.section_headers_len);
    printf("Section header size: %zd\n", sizeof(struct WinSectionHeader));
    printf(
        "Symbol table offset: 0x%x\n",
        pe_data.winpe_header->image_file_header.pointer_to_symbol_table
    );
    printf(
        "Symbol length: %d\n",
        pe_data.winpe_header->image_file_header.number_of_symbols
    );

    /* Section headers */

    printf("\nSection Headers (%zd):\n", pe_data.section_headers_len);
    for (size_t i = 0; i < pe_data.section_headers_len; i++) {
        struct WinSectionHeader *section_header = &pe_data.section_headers[i];
        size_t file_offset = section_header->file_offset;
        size_t win_permissions = section_header->characteristics >> 28;
        char *read = win_permissions & 4 ? "r" : "-";
        char *write = win_permissions & 8 ? "w" : "-";
        char *execute = win_permissions & 2 ? "e" : "-";
        char *visibility = win_permissions & 1 ? "s" : "p";
        printf(
            "%zd, %s, 0x%x, 0x%x, 0x%zx, %s%s%s%s\n",
            i,
            section_header->name,
            section_header->virtual_size,
            section_header->virtual_base_address,
            file_offset,
            read,
            write,
            execute,
            visibility
        );
    }

    /* Imports */

    char *import_section_name =
        pe_data.import_section ? (char *)pe_data.import_section->name : "N/A";
    printf(
        "\nImports (%zd) (%s):\n",
        pe_data.import_dir_entries_len,
        import_section_name
    );
    for (size_t i = 0; i < pe_data.import_dir_entries_len; i++) {
        struct ImportDirectoryEntry *dir_entry = &pe_data.import_dir_entries[i];
        printf(
            "%s (%zd):\n", dir_entry->lib_name, dir_entry->import_entries_len
        );
        for (size_t j = 0; j < dir_entry->import_entries_len; j++) {
            struct ImportEntry *import_entry = &dir_entry->import_entries[j];
            printf(
                "%zd: 0x%zx, %s\n", j, import_entry->address, import_entry->name
            );
        }
    }

    printf("\nImport Address Table (%zd):\n", pe_data.import_address_table_len);
    for (size_t i = 0; i < pe_data.import_address_table_len; i++) {
        struct ImportAddressEntry *iat_entry = &pe_data.import_address_table[i];
        printf(
            "%zd: 0x%zx:0x%zx %s\n",
            i,
            iat_entry->key,
            iat_entry->value,
            iat_entry->import_name
        );
    }

    /* Exports */

    const char *export_section_name =
        pe_data.export_section_name ? pe_data.export_section_name : "N/A";
    printf(
        "\nExports (%zd) (%s):\n",
        pe_data.export_entries_len,
        export_section_name
    );
    for (size_t i = 0; i < pe_data.export_entries_len; i++) {
        struct ExportEntry *export_entry = &pe_data.export_entries[i];
        printf(
            "%zd: 0x%zx: %s\n", i, export_entry->address, export_entry->name
        );
    }

    /* Relocations */

    printf("\nRelocations (%zd):\n", pe_data.relocations.length);
    for (size_t i = 0; i < pe_data.relocations.length; i++) {
        struct RelocationEntry *relocation = &pe_data.relocations.data[i];
        size_t offset = relocation->block_page_rva + relocation->offset;
        char *type = relocation->type == 0x00 ? "IMAGE_REL_BASED_ABSOLUTE"
            : relocation->type == 0x0a        ? "IMAGE_REL_BASED_DIR64"
                                              : "UNKNOWN";
        printf("%zd: 0x%zx, %s\n", i, offset, type);
    }

    /* Symbols */

    printf("\nSymbols (%zd):\n", pe_data.symbols_len);
    if (show_symbols) {
        for (size_t i = 0; i < pe_data.symbols_len; i++) {
            struct WinSymbol *symbol = &pe_data.symbols[i];
            size_t section_start = 0;
            if (symbol->section_index < pe_data.section_headers_len) {
                struct WinSectionHeader *section_header =
                    &pe_data.section_headers[symbol->section_index];
                section_start = section_header->virtual_base_address;
            }
            char *symbol_type = symbol->type == 0x20 ? "FUNCTION" : "-";
            char *symbol_class = symbol->storage_class == 0x02 ? "EXTERNAL"
                : symbol->storage_class == 0x03                ? "STATIC"
                                                               : "-";

            printf(
                "%zd: 0x%zx: 0x%x %s, %s %s\n",
                symbol->raw_index,
                section_start,
                symbol->value,
                symbol->name,
                symbol_type,
                symbol_class
            );
        }
    }
}
