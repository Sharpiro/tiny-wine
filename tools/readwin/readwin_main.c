#include "../../src/loader//pe_tools.h"
#include "../../src/tiny_c/tiny_c.h"
#include <fcntl.h>
#include <stddef.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        EXIT("file required\n");
    }

    char *filename = argv[1];
    int32_t fd = tiny_c_open(filename, O_RDONLY);
    if (fd < 0) {
        EXIT("file not found\n");
    }

    struct PeData pe_data;
    if (!get_pe_data(fd, &pe_data)) {
        EXIT("failed getting PE data\n");
    }
    tiny_c_close(fd);

    size_t dos_magic = pe_data.dos_header->e_magic;
    size_t pe_magic = pe_data.winpe_header->image_optional_header.magic;
    char *class = pe_magic == 0x10b ? "PE32" : "PE32+";
    size_t image_header_start = (size_t)pe_data.dos_header->e_lfanew;
    size_t section_headers_start =
        image_header_start + sizeof(struct WinPEHeader);
    size_t image_base = pe_data.winpe_header->image_optional_header.image_base;
    size_t base_of_code =
        pe_data.winpe_header->image_optional_header.base_of_code;

    tiny_c_printf("PE Header:\n");
    tiny_c_printf("DOS magic: %s\n", (char *)&dos_magic);
    tiny_c_printf("PE magic: %x\n", pe_magic);
    tiny_c_printf("Class: %s\n", class);
    tiny_c_printf("Image base: %x\n", image_base);
    tiny_c_printf("Base of code: %x\n", base_of_code);
    tiny_c_printf("Entry point address: %x\n", pe_data.entrypoint);
    tiny_c_printf("Section headers start: %x\n", section_headers_start);
    tiny_c_printf("Section headers length: %d\n", pe_data.section_headers_len);
    tiny_c_printf("Section header size: %d\n", sizeof(struct WinSectionHeader));

    tiny_c_printf("\nSection Headers:\n");
    for (size_t i = 0; i < pe_data.section_headers_len; i++) {
        struct WinSectionHeader *section_header = &pe_data.section_headers[i];
        size_t address = image_base + section_header->virtual_address;
        size_t file_offset = section_header->pointer_to_raw_data;
        size_t permissions = section_header->characteristics >> 28;
        char *read = permissions & 4 ? "r" : "-";
        char *write = permissions & 8 ? "w" : "-";
        char *execute = permissions & 2 ? "e" : "-";
        tiny_c_printf(
            "%d, %s, %x, %x, %x, %s%s%s\n",
            i,
            section_header->name,
            section_header->virtual_size,
            address,
            file_offset,
            read,
            write,
            execute
        );
    }

    tiny_c_printf(
        "\nImport Address Table: %x, %d\n",
        pe_data.import_address_table_offset,
        pe_data.import_address_table_len
    );
    for (size_t i = 0; i < pe_data.import_address_table_len; i++) {
        struct KeyValue *iat_entry = &pe_data.import_address_table[i];
        tiny_c_printf("%x:%x\n", iat_entry->key, iat_entry->value);
    }

    tiny_c_printf("\nSection .idata:\n");
    for (size_t i = 0; i < pe_data.import_dir_entries_len; i++) {
        struct ImportDirectoryEntry *dir_entry = &pe_data.import_dir_entries[i];
        tiny_c_printf("Lib: %s:\n", dir_entry->lib_name);
        for (size_t i = 0; i < dir_entry->import_entries_len; i++) {
            struct ImportEntry *import_entry = &dir_entry->import_entries[i];
            tiny_c_printf(
                "Import: %s, %x\n", import_entry->name, import_entry->address
            );
        }
    }
}
