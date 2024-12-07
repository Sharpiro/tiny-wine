
#include "./pe_tools.h"
#include "../tiny_c/tiny_c.h"
#include "loader_lib.h"
#include <stddef.h>

bool get_pe_data(int32_t fd, struct PeData *pe_data) {
    if (pe_data == NULL) {
        BAIL("pe_data was null\n");
    }

    int8_t *file_buffer = loader_malloc_arena(1000);
    tiny_c_read(fd, file_buffer, 1000);

    struct ImageDosHeader *dos_header = (struct ImageDosHeader *)file_buffer;
    size_t image_header_start = (size_t)dos_header->e_lfanew;
    struct WinPEHeader *winpe_header =
        (struct WinPEHeader *)(file_buffer + image_header_start);
    size_t image_base = winpe_header->image_optional_header.image_base;
    size_t entrypoint =
        image_base + winpe_header->image_optional_header.address_of_entry_point;

    size_t section_headers_start =
        (size_t)dos_header->e_lfanew + sizeof(struct WinPEHeader);
    size_t section_headers_len =
        winpe_header->image_file_header.number_of_sections;
    struct WinSectionHeader *section_headers =
        (struct WinSectionHeader *)(file_buffer + section_headers_start);

    struct WinSectionHeader *import_data_header = NULL;
    for (size_t i = 0; i < section_headers_len; i++) {
        if (tiny_c_strcmp((const char *)section_headers[i].name, ".idata") ==
            0) {
            import_data_header = &section_headers[i];
        }
    }
    if (import_data_header == NULL) {
        BAIL(".idata section not found");
    }

    // @todo: invalid address, need to load from file
    struct ImportDirectoryEntry *import_dir_entries =
        (struct ImportDirectoryEntry *)(image_base +
                                        import_data_header->pointer_to_raw_data
        );
    size_t import_dir_entries_len = 0;
    for (size_t i = 0; true; i++) {
        struct ImportDirectoryEntry *import_dir_entry = &import_dir_entries[i];
        const uint8_t ENTRY_COMPARE[IMPORT_DIRECTORY_ENTRY_SIZE] = {0};
        if (tiny_c_memcmp(
                import_dir_entry, ENTRY_COMPARE, IMPORT_DIRECTORY_ENTRY_SIZE
            ) == 0) {
            break;
        }
        import_dir_entries_len++;
    }

    *pe_data = (struct PeData){
        .dos_header = dos_header,
        .winpe_header = winpe_header,
        .entrypoint = entrypoint,
        .section_headers = section_headers,
        .section_headers_len = section_headers_len,
        .import_dir_entries = import_dir_entries,
        .import_dir_entries_len = import_dir_entries_len,
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
