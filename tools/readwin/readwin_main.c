#include "../../src/loader/pe_tools.h"
#include "../../src/tiny_c/tiny_c.h"
#include <fcntl.h>
#include <stddef.h>

int main(int argc, char **argv) {
    if (argc < 2 || tiny_c_memcmp(argv[1], "--", 2) == 0) {
        EXIT("Usage: readwin <file> [-s]\n");
    }

    char *filename = argv[1];
    int32_t fd = tiny_c_open(filename, O_RDONLY);
    if (fd < 0) {
        EXIT("file not found\n");
    }

    const bool show_symbols = argc >= 3 && tiny_c_strcmp(argv[2], "-s") == 0;

    struct PeData pe_data;
    if (!get_pe_data(fd, &pe_data)) {
        EXIT("failed getting PE data\n");
    }
    tiny_c_close(fd);

    size_t dos_magic = pe_data.dos_header->magic;
    size_t pe_magic = pe_data.winpe_header->image_optional_header.magic;
    char *class = pe_magic == PE32_MAGIC ? "PE32" : "PE32+";
    bool is_64_bit = pe_magic == PE32_PLUS_MAGIC;
    size_t image_header_start = (size_t)pe_data.dos_header->e_lfanew;
    size_t section_headers_start =
        image_header_start + sizeof(struct WinPEHeader);
    size_t base_of_code =
        pe_data.winpe_header->image_optional_header.base_of_code;
    size_t image_base = is_64_bit
        ? pe_data.winpe_header->image_optional_header.image_base
        : pe_data.winpe_header->image_optional_header_32.image_base;

    /* General information */

    tiny_c_printf("PE Header:\n");
    tiny_c_printf("DOS magic: %s\n", (char *)&dos_magic);
    tiny_c_printf("PE magic: %x\n", pe_magic);
    tiny_c_printf("Class: %s\n", class);
    tiny_c_printf("Base of code: %x\n", base_of_code);
    tiny_c_printf("Image base: %x\n", image_base);
    tiny_c_printf("Entry point address: %x\n", pe_data.entrypoint);
    tiny_c_printf("Section headers start: %x\n", section_headers_start);
    tiny_c_printf("Section headers length: %d\n", pe_data.section_headers_len);
    tiny_c_printf("Section header size: %d\n", sizeof(struct WinSectionHeader));
    tiny_c_printf(
        "Symbol table offset: %x\n",
        pe_data.winpe_header->image_file_header.pointer_to_symbol_table
    );
    tiny_c_printf(
        "Symbol length: %d\n",
        pe_data.winpe_header->image_file_header.number_of_symbols
    );

    /* Section headers */

    tiny_c_printf("\nSection Headers (%d):\n", pe_data.section_headers_len);
    for (size_t i = 0; i < pe_data.section_headers_len; i++) {
        struct WinSectionHeader *section_header = &pe_data.section_headers[i];
        size_t file_offset = section_header->file_offset;
        size_t win_permissions = section_header->characteristics >> 28;
        char *read = win_permissions & 4 ? "r" : "-";
        char *write = win_permissions & 8 ? "w" : "-";
        char *execute = win_permissions & 2 ? "e" : "-";
        char *visibility = win_permissions & 1 ? "s" : "p";
        tiny_c_printf(
            "%d, %s, %x, %x, %x, %s%s%s%s\n",
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

    const char *import_section_name =
        pe_data.import_section_name ? pe_data.import_section_name : "N/A";
    tiny_c_printf(
        "\nImports (%d) (%s):\n",
        pe_data.import_dir_entries_len,
        import_section_name
    );
    for (size_t i = 0; i < pe_data.import_dir_entries_len; i++) {
        struct ImportDirectoryEntry *dir_entry = &pe_data.import_dir_entries[i];
        tiny_c_printf(
            "%s (%d):\n", dir_entry->lib_name, dir_entry->import_entries_len
        );
        for (size_t j = 0; j < dir_entry->import_entries_len; j++) {
            struct ImportEntry *import_entry = &dir_entry->import_entries[j];
            tiny_c_printf(
                "%d: %x, %s\n", j, import_entry->address, import_entry->name
            );
        }
    }

    tiny_c_printf(
        "\nImport Address Table (%d):\n", pe_data.import_address_table_len
    );
    for (size_t i = 0; i < pe_data.import_address_table_len; i++) {
        struct ImportAddressEntry *iat_entry = &pe_data.import_address_table[i];
        tiny_c_printf(
            "%d: %x:%x %s\n",
            i,
            iat_entry->key,
            iat_entry->value,
            iat_entry->import_name
        );
    }

    /* Exports */

    const char *export_section_name =
        pe_data.export_section_name ? pe_data.export_section_name : "N/A";
    tiny_c_printf(
        "\nExports (%d) (%s):\n",
        pe_data.export_entries_len,
        export_section_name
    );
    for (size_t i = 0; i < pe_data.export_entries_len; i++) {
        struct ExportEntry *export_entry = &pe_data.export_entries[i];
        tiny_c_printf(
            "%d: %x: %s\n", i, export_entry->address, export_entry->name
        );
    }

    /* Symbols */

    tiny_c_printf("\nSymbols (%d):\n", pe_data.symbols_len);
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

            tiny_c_printf(
                "%d: %x: %x %s, %s %s\n",
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
