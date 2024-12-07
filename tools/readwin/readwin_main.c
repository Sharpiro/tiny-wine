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

    tiny_c_printf("PE Header:\n", filename);
    tiny_c_printf("DOS magic: %s\n", (char *)&dos_magic);
    tiny_c_printf("PE magic: %x\n", pe_magic);
    tiny_c_printf("Class: %s\n", class);
    tiny_c_printf("Image base: %x\n", image_base);
    tiny_c_printf("Base of code: %x\n", base_of_code);
    tiny_c_printf("Entry point address: %x\n", pe_data.entrypoint);
    tiny_c_printf("Section headers start: %x\n", section_headers_start);
    tiny_c_printf("Section headers length: %d\n", pe_data.section_headers_len);
    tiny_c_printf("Section header size: %d\n", sizeof(struct WinSectionHeader));

    tiny_c_printf("\nSection Headers:\n", filename);
    for (size_t i = 0; i < pe_data.section_headers_len; i++) {
        struct WinSectionHeader *section_header = &pe_data.section_headers[i];
        tiny_c_printf(
            "%d, %s, %x, %x, %x, %x\n",
            i,
            section_header->name,
            section_header->virtual_size,
            image_base + section_header->virtual_address,
            section_header->pointer_to_raw_data,
            section_header->characteristics
        );
    }
}
