#pragma once

#include "elf_tools.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DOS_MAGIC 0x5a4d
#define DYNAMIC_CALLBACK_TRAMPOLINE_SIZE 7

static_assert(DYNAMIC_CALLBACK_TRAMPOLINE_SIZE <= 0x08);

struct ImageDosHeader {
    uint16_t magic;      // Magic number ("MZ")
    uint16_t e_cblp;     // Bytes on last page of file
    uint16_t e_cp;       // Pages in file
    uint16_t e_crlc;     // Relocations
    uint16_t e_cparhdr;  // Size of header in paragraphs
    uint16_t e_minalloc; // Minimum extra paragraphs needed
    uint16_t e_maxalloc; // Maximum extra paragraphs needed
    uint16_t e_ss;       // Initial (relative) SS value
    uint16_t e_sp;       // Initial SP value
    uint16_t e_csum;     // Checksum
    uint16_t e_ip;       // Initial IP value
    uint16_t e_cs;       // Initial (relative) CS value
    uint16_t e_lfarlc;   // File address of relocation table
    uint16_t e_ovno;     // Overlay number
    uint16_t e_res[4];   // Reserved words
    uint16_t e_oemid;    // OEM identifier
    uint16_t e_oeminfo;  // OEM information
    uint16_t e_res2[10]; // Reserved words
    int32_t e_lfanew;    // File address of new exe header
};

struct ImageFileHeader {
    uint16_t machine;                 // Target machine type
    uint16_t number_of_sections;      // Number of sections
    uint32_t time_date_stamp;         // Timestamp
    uint32_t pointer_to_symbol_table; // Pointer to symbol table (deprecated)
    uint32_t number_of_symbols;       // Number of symbols (deprecated)
    uint16_t size_of_optional_header; // Size of the optional header
    uint16_t characteristics;         // Flags indicating file characteristics
};

struct ImageDataDirectory {
    uint32_t virtual_address; // Relative virtual address (RVA) of the directory
    uint32_t size;            // Size of the directory in bytes
};

#define PE32_MAGIC 0x010b
#define PE32_PLUS_MAGIC 0x020b

struct ImageOptionalHeader {
    uint16_t magic;                          // Identifies PE32 or PE32+
    uint8_t major_linker_version;            // Linker version
    uint8_t minor_linker_version;            // Linker version
    uint32_t size_of_code;                   // Size of .text section
    uint32_t size_of_initialized_data;       // Size of initialized data
    uint32_t size_of_uninitialized_data;     // Size of uninitialized data
    uint32_t address_of_entry_point;         // RVA of entry point
    uint32_t base_of_code;                   // RVA of code section
    uint64_t image_base;                     // Preferred load address
    uint32_t section_alignment;              // Alignment of sections in memory
    uint32_t file_alignment;                 // Alignment of sections in file
    uint16_t major_operating_system_version; // Minimum OS version
    uint16_t minor_operating_system_version;
    uint16_t major_image_version; // Image version
    uint16_t minor_image_version;
    uint16_t major_subsystem_version; // Subsystem version
    uint16_t minor_subsystem_version;
    uint32_t win32_version_value;     // Reserved, should be 0
    uint32_t size_of_image;           // Total image size
    uint32_t size_of_headers;         // Combined size of headers
    uint32_t checksum;                // Checksum of the image
    uint16_t subsystem;               // Subsystem (e.g., GUI, Console)
    uint16_t dll_characteristics;     // DLL characteristics flags
    uint64_t size_of_stack_reserve;   // Stack reserve size
    uint64_t size_of_stack_commit;    // Stack commit size
    uint64_t size_of_heap_reserve;    // Heap reserve size
    uint64_t size_of_heap_commit;     // Heap commit size
    uint32_t loader_flags;            // Loader flags
    uint32_t number_of_rva_and_sizes; // Number of data directory entries
    struct ImageDataDirectory data_directory[16]; // Array of data directories
};

#define DATA_DIR_IAT_INDEX 12

struct WinPEHeader {
    uint32_t signature;
    struct ImageFileHeader image_file_header;
    struct ImageOptionalHeader image_optional_header;
};

struct WinSectionHeader {
    uint8_t name[8];       // Section name (8 bytes)
    uint32_t virtual_size; // Total size of the section when loaded into memory
    uint32_t base_address; // Address of the section relative to the image base
    uint32_t size_of_raw_data;       // Size of the section in the file
    uint32_t pointer_to_raw_data;    // File offset
    uint32_t pointer_to_relocations; // File pointer to relocations (deprecated)
    uint32_t
        pointer_to_linenumbers;     // File pointer to line numbers (deprecated)
    uint16_t number_of_relocations; // Number of relocation entries
    uint16_t number_of_linenumbers; // Number of line number entries
    uint32_t characteristics;       // Flags describing section attributes
};

struct ImportEntry {
    const char *name;
    size_t address;
};

struct ImportDirectoryRawEntry {
    uint32_t characteristics;
    uint32_t timestamp;
    uint32_t forwarder_chain;
    uint32_t name_offset;
    uint32_t import_address_table_offset;
};

struct ImportDirectoryEntry {
    size_t import_lookup_table_offset;
    const char *lib_name;
    struct ImportEntry *import_entries;
    size_t import_entries_len;
};

#define IMPORT_DIRECTORY_RAW_ENTRY_SIZE sizeof(struct ImportDirectoryRawEntry)

struct ExportDirectoryRawEntry {
    uint32_t export_flags;
    uint32_t timestamp;
    uint16_t major_version;
    uint16_t minor_version;
    uint32_t name_offset;
    uint32_t ordinal_base;
    uint32_t address_table_len;
    uint32_t name_points_len;
    uint32_t export_address_table_offset;
    uint32_t name_pointer_offset;
    uint32_t ordinal_table_offset;
};

struct ExportEntry {
    size_t address;
    char *name;
};

// @todo: alignment problems on some platforms?
struct RawWinSymbol {
    char name[8];
    int32_t value;
    uint16_t section_number;
    uint16_t type;
    uint8_t storage_class;
    uint8_t auxillary_symbols_len;
} __attribute__((packed));

struct WinSymbol {
    char *name;
    int32_t value;
    uint16_t section_number;
    uint16_t type;
    uint8_t storage_class;
    uint8_t auxillary_symbols_len;
    size_t raw_index;
};

#define SYMBOL_TYPE_FUNCTION 0x20
#define SYMBOL_CLASS_EXTERNAL 0x02
#define SYMBOL_CLASS_STATIC 0x03

struct ImportAddressEntry {
    size_t key;
    size_t value;
    const char *lib_name;
    const char *import_name;
    bool is_variable;
};

struct PeData {
    struct ImageDosHeader *dos_header;
    struct WinPEHeader *winpe_header;
    size_t entrypoint;
    struct WinSectionHeader *section_headers;
    size_t section_headers_len;
    struct ImportDirectoryEntry *import_dir_entries;
    size_t import_dir_entries_len;
    size_t import_address_table_offset;
    struct ImportAddressEntry *import_address_table;
    size_t import_address_table_len;
    struct ExportEntry *export_entries;
    size_t export_entries_len;
    struct WinSymbol *symbols;
    size_t symbols_len;
};

bool get_pe_data(int32_t fd, struct PeData *pe_data);

bool get_memory_regions_info_win(
    const struct WinSectionHeader *program_headers,
    size_t program_headers_len,
    size_t address_offset,
    struct MemoryRegionsInfo *memory_regions_info
);

const struct WinSectionHeader *find_win_section_header(
    const struct WinSectionHeader *section_headers,
    size_t section_headers_len,
    const char *name
);

bool map_memory_regions_win(
    int32_t fd, const struct MemoryRegion *regions, size_t regions_len
);

bool map_import_address_table(
    int32_t fd,
    size_t iat_base,
    size_t idata_base,
    size_t image_base,
    struct ImportAddressEntry *import_address_table,
    size_t import_address_table_len,
    size_t dynamic_callback_trampoline,
    size_t *iat_runtime_base
);

const struct WinRuntimeObject *find_runtime_object(
    const struct WinRuntimeObject *runtime_objects,
    size_t runtime_objects_len,
    const char *name
);

const struct WinSymbol *find_runtime_symbol(
    const struct WinSymbol *symbols, size_t symbols_len, const char *name
);
