#include "../tiny_c/tiny_c.h"
#include "./pe_tools.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

CREATE_LIST_STRUCT(WinRuntimeObject)

#define IAT_BASE_START 0x7d7e0000

struct WinRuntimeObject runtime_exe;
struct RuntimeObject *lib_ntdll_object;
WinRuntimeObjectList shared_libraries = {};
size_t current_iat_base = IAT_BASE_START;
size_t current_iat_offset = 0;

// @todo: x86 seems to not need 'frame_start' since frame pointer can be 0'd
//        does arm?
static void run_asm(
    [[maybe_unused]] size_t frame_start,
    size_t stack_start,
    size_t program_entry
) {
    __asm__("mov rbx, 0x00\n"
            "mov rsp, %[stack_start]\n"

            /* clear 'PF' flag */
            "mov r15, 0xff\n"
            "xor r15, 1\n"

            "mov rax, 0x00\n"
            "mov rcx, 0x00\n"
            "mov rdx, 0x00\n"
            "mov rsi, 0x00\n"
            "mov rdi, 0x00\n"
            "mov r8, 0x00\n"
            "mov r9, 0x00\n"
            "mov r10, 0x00\n"
            "mov r11, 0x00\n"
            "mov r12, 0x00\n"
            "mov r13, 0x00\n"
            "mov r14, 0x00\n"
            "mov r15, 0x00\n"
            :
            : [stack_start] "r"(stack_start));

    __asm__("mov rbp, 0\n"
            "jmp %[program_entry]\n"
            :
            : [program_entry] "r"(program_entry)
            : "rax");
}

// @todo: replace with sys_exit
__attribute__((naked)) void win_loader_end(void) {
    __asm__("mov rdi, rax\n"
            "mov rax, 0x3c\n"
            "syscall\n");
}

static void dynamic_callback(void) {
    size_t *rbp;
    __asm__("mov %0, rbp" : "=r"(rbp));
    size_t p1;
    __asm__("mov %0, rcx" : "=r"(p1));
    size_t p2;
    __asm__("mov %0, rdx" : "=r"(p2));
    size_t p3;
    __asm__("mov %0, r8" : "=r"(p3));
    size_t p4;
    __asm__("mov %0, r9" : "=r"(p4));
    size_t p5 = rbp[7];
    size_t p6 = rbp[8];

    size_t dyn_trampoline_end = *(rbp + 1);
    size_t dyn_trampoline_start =
        dyn_trampoline_end - DYNAMIC_CALLBACK_TRAMPOLINE_SIZE;
    size_t iat_runtime_base = dyn_trampoline_start / 0x1000 * 0x1000;
    size_t func_iat_value_temp = dyn_trampoline_start - IAT_BASE_START;

    // WinRuntimeObject *shared_lib = NULL;
    size_t func_iat_key = 0;
    size_t func_iat_value = 0;
    if (iat_runtime_base == runtime_exe.iat_runtime_base) {
        for (size_t i = 0; i < runtime_exe.pe_data.import_address_table_len;
             i++) {
            struct KeyValue *iat_entry =
                &runtime_exe.pe_data.import_address_table[i];
            size_t curr_func_iat_value =
                func_iat_value_temp - runtime_exe.iat_runtime_offset;
            if (iat_entry->value == curr_func_iat_value) {
                func_iat_key = iat_entry->key;
                func_iat_value = iat_entry->value;
                break;
            }
        }
    } else {
        for (size_t i = 0; i < shared_libraries.length; i++) {
            WinRuntimeObject *curr_shared_lib = &shared_libraries.data[i];
            if (iat_runtime_base != curr_shared_lib->iat_runtime_base) {
                continue;
            }

            for (size_t i = 0;
                 i < curr_shared_lib->pe_data.import_address_table_len;
                 i++) {
                struct KeyValue *iat_entry =
                    &curr_shared_lib->pe_data.import_address_table[i];
                size_t curr_func_iat_value =
                    func_iat_value_temp - curr_shared_lib->iat_runtime_offset;
                if (iat_entry->value == curr_func_iat_value) {
                    func_iat_key = iat_entry->key;
                    func_iat_value = iat_entry->value;
                    break;
                }
            }
            break;
        }
    }
    if (func_iat_value == 0) {
        EXIT("func_iat_value '%x' not found\n", func_iat_value);
    }

    LOADER_LOG(
        "Dynamic linker callback hit, %x:%x\n", func_iat_key, func_iat_value
    );

    const char *lib_name = NULL;
    struct ImportEntry *import_entry = NULL;
    for (size_t i = 0; i < runtime_exe.pe_data.import_dir_entries_len; i++) {
        struct ImportDirectoryEntry *dir_entry =
            &runtime_exe.pe_data.import_dir_entries[i];
        for (size_t i = 0; i < dir_entry->import_entries_len; i++) {
            struct ImportEntry *current_entry = &dir_entry->import_entries[i];
            if (current_entry->address == func_iat_value) {
                lib_name = dir_entry->lib_name;
                import_entry = current_entry;
            }
        }
    }
    if (import_entry == NULL) {
        EXIT("import_entry %x not found\n", func_iat_value);
    };

    LOADER_LOG(
        "%s: %s(%x, %x, %x, %x, %x, %x)\n",
        lib_name,
        import_entry->name,
        p1,
        p2,
        p3,
        p4,
        p5,
        p6
    );

    WinRuntimeExport *function_export = NULL;
    if (tiny_c_strcmp(lib_name, "ntdll.dll") == 0) {
        for (size_t i = 0; i < lib_ntdll_object->runtime_symbols.length; i++) {
            RuntimeSymbol *curr_symbol =
                &lib_ntdll_object->runtime_symbols.data[i];
            if (tiny_c_strcmp(curr_symbol->name, import_entry->name) == 0) {
                *function_export = (WinRuntimeExport){
                    .address = curr_symbol->value,
                    .name = curr_symbol->name,
                };
                break;
            }
        }
    } else {
        for (size_t i = 0; i < shared_libraries.length; i++) {
            WinRuntimeObject *shared_lib = &shared_libraries.data[i];
            for (size_t i = 0; i < shared_lib->function_exports.length; i++) {
                WinRuntimeExport *curr_func_export =
                    &shared_lib->function_exports.data[i];
                if (tiny_c_strcmp(curr_func_export->name, import_entry->name) ==
                    0) {
                    function_export = curr_func_export;
                    break;
                }
            }
        }
    }
    if (function_export == NULL) {
        EXIT("expected runtime function\n");
    }

    LOADER_LOG("runtime function %x\n", function_export->address);

    LOADER_LOG("completed dynamic linking\n");

    __asm__("mov r15, %0\n"
            "mov rdi, %1\n"
            "mov rsi, %2\n"
            "mov rdx, %3\n"
            "mov rcx, %4\n"
            "mov r8, %5\n"
            "mov r9, %6\n"
            "mov rsp, rbp\n"
            "pop rbp\n"
            "jmp r15\n" ::"r"(function_export->address),
            "r"(p1),
            "r"(p2),
            "r"(p3),
            "r"(p4),
            "r"(p5),
            "r"(p6)
            : "r15", "rdi", "rsi", "rdx", "rcx", "r8", "r9");
}

__attribute__((naked)) static void dynamic_callback_trampoline(void) {
    __asm__("call %0\n" ::"r"(dynamic_callback));
}

static bool initialize_lib_ntdll(struct RuntimeObject *lib_ntdll_object) {
    const char *LIB_NTDLL_SO_NAME = "libntdll.so";

    int32_t ntdll_file = tiny_c_open(LIB_NTDLL_SO_NAME, O_RDONLY);
    if (ntdll_file == -1) {
        BAIL("failed opening libntdll.so\n");
    }

    struct ElfData ntdll_elf;
    if (!get_elf_data(ntdll_file, &ntdll_elf)) {
        BAIL("failed getting elf data\n");
    }
    if (ntdll_elf.dynamic_data == NULL) {
        BAIL("Expected shared library to have dynamic data\n");
    }

    // @todo: need runtime tracking for other shared libs
    size_t dynamic_lib_offset = LOADER_SHARED_LIB_START;
    struct MemoryRegionsInfo memory_regions_info;
    if (!get_memory_regions_info_x86(
            ntdll_elf.program_headers,
            ntdll_elf.header.e_phnum,
            dynamic_lib_offset,
            &memory_regions_info
        )) {
        BAIL("failed getting memory regions\n");
    }

    LOADER_LOG("Mapping library memory regions\n");
    if (!map_memory_regions(
            ntdll_file,
            memory_regions_info.regions,
            memory_regions_info.regions_len
        )) {
        BAIL("loader lib map memory regions failed\n");
    }

    tiny_c_close(ntdll_file);

    // @todo: computed not initialized
    uint8_t *bss = 0;
    size_t bss_len = 0;
    const struct SectionHeader *bss_section_header = find_section_header(
        ntdll_elf.section_headers, ntdll_elf.section_headers_len, ".bss"
    );
    if (bss_section_header != NULL) {
        bss = (uint8_t *)(dynamic_lib_offset + bss_section_header->addr);
        bss_len = bss_section_header->size;
    }

    struct RuntimeRelocation *runtime_func_relocations;
    size_t runtime_func_relocations_len;
    if (!get_function_relocations(
            ntdll_elf.dynamic_data,
            dynamic_lib_offset,
            &runtime_func_relocations,
            &runtime_func_relocations_len
        )) {
        BAIL("get_function_relocations failed\n");
    }

    RuntimeSymbolList runtime_symbols = {
        .allocator = loader_malloc_arena,
    };
    if (!get_symbols(
            ntdll_elf.dynamic_data, dynamic_lib_offset, &runtime_symbols
        )) {
        BAIL("failed getting symbols\n");
    }

    *lib_ntdll_object = (struct RuntimeObject){
        .name = LIB_NTDLL_SO_NAME,
        .dynamic_offset = LOADER_SHARED_LIB_START,
        .elf_data = ntdll_elf,
        .memory_regions_info = memory_regions_info,
        .runtime_func_relocations = runtime_func_relocations,
        .runtime_func_relocations_len = runtime_func_relocations_len,
        .bss = bss,
        .bss_len = bss_len,
        .runtime_symbols = runtime_symbols,

    };

    return true;
}

static bool initialize_dynamic_data(
    struct PeData *inferior_executable, WinRuntimeObjectList *shared_libraries
) {
    for (size_t i = 0; i < inferior_executable->import_dir_entries_len; i++) {
        /* Map shared libraries */

        struct ImportDirectoryEntry *dir_entry =
            &inferior_executable->import_dir_entries[i];
        const char *shared_lib_name = dir_entry->lib_name;
        LOADER_LOG("mapping shared library '%s'\n", shared_lib_name);
        int32_t shared_lib_file = tiny_c_open(shared_lib_name, O_RDONLY);
        if (shared_lib_file == -1) {
            BAIL("failed opening shared lib '%s'\n", shared_lib_name);
        }

        struct PeData shared_lib_pe;
        if (!get_pe_data(shared_lib_file, &shared_lib_pe)) {
            BAIL("failed getting data for shared lib '%s'\n", shared_lib_name);
        }

        // @todo: provide offset by loader?
        size_t shared_lib_image_base =
            shared_lib_pe.winpe_header->image_optional_header.image_base;
        struct MemoryRegionsInfo memory_regions_info;
        if (!get_memory_regions_info_win(
                shared_lib_pe.section_headers,
                shared_lib_pe.section_headers_len,
                shared_lib_image_base,
                &memory_regions_info
            )) {
            BAIL("failed getting memory regions\n");
        }

        LOADER_LOG("Mapping library memory regions\n");

        if (!map_memory_regions_win(
                shared_lib_file,
                memory_regions_info.regions,
                memory_regions_info.regions_len
            )) {
            BAIL("loader lib map memory regions failed\n");
        }

        tiny_c_close(shared_lib_file);
        if (!print_memory_regions()) {
            BAIL("print_memory_regions failed\n");
        }

        WinRuntimeExportList runtime_exports = {
            .allocator = loader_malloc_arena,
        };
        for (size_t i = 0; i < dir_entry->import_entries_len; i++) {
            const struct ImportEntry *import_entry =
                &dir_entry->import_entries[i];
            const struct ExportEntry *export_entry = NULL;
            for (size_t i = 0; i < shared_lib_pe.export_entries_len; i++) {
                const struct ExportEntry *curr_export_entry =
                    &shared_lib_pe.export_entries[i];
                if (tiny_c_strcmp(
                        import_entry->name, curr_export_entry->name
                    ) == 0) {
                    export_entry = curr_export_entry;
                }
            }
            if (export_entry == NULL) {
                BAIL("import '%s' not found", import_entry->name);
            }

            struct WinRuntimeExport runtime_relocation = {
                .address = shared_lib_image_base + export_entry->address,
                .name = export_entry->name,
            };
            WinRuntimeExportList_add(&runtime_exports, runtime_relocation);
        }

        /* Map Import Address Table */

        const struct WinSectionHeader *idata_header = find_win_section_header(
            shared_lib_pe.section_headers,
            shared_lib_pe.section_headers_len,
            ".idata"
        );
        size_t iat_runtime_base;
        map_import_address_table(
            shared_lib_file,
            current_iat_base,
            idata_header->base_address,
            shared_lib_image_base,
            shared_lib_pe.import_address_table_offset,
            shared_lib_pe.import_address_table_len,
            (size_t)dynamic_callback_trampoline,
            &iat_runtime_base
        );
        size_t iat_runtime_offset = current_iat_offset;
        current_iat_base += 0x1000;
        current_iat_offset += 0x1000;

        struct WinRuntimeObject shared_lib = {
            .name = dir_entry->lib_name,
            .pe_data = shared_lib_pe,
            .memory_regions_info = memory_regions_info,
            .function_exports = runtime_exports,
            .iat_runtime_base = iat_runtime_base,
            .iat_runtime_offset = iat_runtime_offset,
        };
        WinRuntimeObjectList_add(shared_libraries, shared_lib);
    }

    return true;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        return -1;
    }

    char *filename = argv[1];
    LOADER_LOG("Starting winloader, %s, %d\n", filename, argc);

    int32_t pid = tiny_c_get_pid();
    LOADER_LOG("pid: %d\n", pid);

    if (tiny_c_munmap(0x400000, 0x1000)) {
        tiny_c_fprintf(STDERR, "munmap of self failed\n");
        return -1;
    }

    int32_t fd = tiny_c_open(filename, O_RDONLY);
    if (fd < 0) {
        tiny_c_fprintf(
            STDERR,
            "file error, %d, %s\n",
            tinyc_errno,
            tinyc_strerror(tinyc_errno)
        );
        return -1;
    }

    struct PeData pe_exe;
    if (!get_pe_data(fd, &pe_exe)) {
        tiny_c_fprintf(STDERR, "error parsing pe data\n");
        return -1;
    }

    size_t image_base = pe_exe.winpe_header->image_optional_header.image_base;
    struct MemoryRegionsInfo memory_regions_info;
    if (!get_memory_regions_info_win(
            pe_exe.section_headers,
            pe_exe.section_headers_len,
            image_base,
            &memory_regions_info
        )) {
        EXIT("failed getting memory regions\n");
    }

    if (!map_memory_regions_win(
            fd, memory_regions_info.regions, memory_regions_info.regions_len
        )) {
        EXIT("map_memory_regions_win failed\n");
    }

    /* Map Import Address Table */

    const struct WinSectionHeader *idata_header = find_win_section_header(
        pe_exe.section_headers, pe_exe.section_headers_len, ".idata"
    );

    size_t iat_runtime_base;
    map_import_address_table(
        fd,
        current_iat_base,
        idata_header->base_address,
        image_base,
        pe_exe.import_address_table_offset,
        pe_exe.import_address_table_len,
        (size_t)dynamic_callback_trampoline,
        &iat_runtime_base
    );
    current_iat_base += 0x1000;
    current_iat_offset += 0x1000;

    /* Load libntdll.so */

    lib_ntdll_object = loader_malloc_arena(sizeof(struct RuntimeObject));
    if (!initialize_lib_ntdll(lib_ntdll_object)) {
        EXIT("initialize_lib_ntdll failed\n");
    }

    /* Load dlls */

    shared_libraries = (WinRuntimeObjectList){
        .allocator = loader_malloc_arena,
    };
    if (!initialize_dynamic_data(&pe_exe, &shared_libraries)) {
        EXIT("initialize_dynamic_data failed\n");
    }

    print_memory_regions();

    runtime_exe = (struct WinRuntimeObject){
        .name = filename,
        .pe_data = pe_exe,
        .memory_regions_info = memory_regions_info,
        .function_exports = {},
        .iat_runtime_base = iat_runtime_base,
        .iat_runtime_offset = 0,
    };

    /* Jump to program */

    size_t *frame_pointer = (size_t *)argv - 1;
    size_t *inferior_frame_pointer = frame_pointer + 1;
    *inferior_frame_pointer = (size_t)(argc - 1);
    size_t *stack_start = inferior_frame_pointer;
    *stack_start = (size_t)win_loader_end;
    LOADER_LOG("entrypoint: %x\n", pe_exe.entrypoint);
    LOADER_LOG("frame_pointer: %x\n", frame_pointer);
    LOADER_LOG("stack_start: %x\n", stack_start);
    LOADER_LOG("end func: %x\n", *stack_start);
    LOADER_LOG("------------running program------------\n");

    run_asm(
        (size_t)inferior_frame_pointer, (size_t)stack_start, pe_exe.entrypoint
    );
}
