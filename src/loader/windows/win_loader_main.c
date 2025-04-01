#include "../../dlls/macros.h"
#include "../../dlls/win_type.h"
#include "../../tinyc/tinyc.h"
#include "../../tinyc/tinyc_sys.h"
#include "../linux/loader_lib.h"
#include "../log.h"
#include "../memory_map.h"
#include "./pe_tools.h"
#include "win_loader_lib.h"
#include <asm/prctl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct WinRuntimeObject runtime_exe;
struct RuntimeObject *lib_ntdll;
WinRuntimeObjectList shared_libraries = {};
size_t initial_global_runtime_iat_region_base = 0;
size_t got_lib_dyn_offset_table[100] = {};
struct PreservedSwapState preserved_swap_state = {};

/*
 * Dynamic callback from Linux to Linux
 */
static void dynamic_callback_linux(void) {
    size_t rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14, r15, *rbp;
    double xmm0, xmm1;
    GET_REGISTERS_SNAPSHOT();

    size_t p7_stack1 = *(rbp + 4);
    size_t p8_stack2 = *(rbp + 5);

    LOGTRACE("---Starting Linux dyn callback---\n");

    size_t *lib_dyn_offset = (size_t *)(*(rbp + 1));
    size_t relocation_index = *(rbp + 2);
    if (lib_dyn_offset == NULL) {
        EXIT("lib_dyn_offset was null\n");
    }

    LOGTRACE("relocation params: %x, %x\n", *lib_dyn_offset, relocation_index);

    struct RuntimeRelocation *runtime_relocation =
        &lib_ntdll->runtime_func_relocations[relocation_index];

    size_t *got_entry = (size_t *)runtime_relocation->offset;
    LOGTRACE("got_entry: %x: %x\n", got_entry, *got_entry);

    const struct RuntimeSymbol *runtime_symbol;
    if (!find_runtime_symbol(
            runtime_relocation->name,
            lib_ntdll->runtime_symbols.data,
            lib_ntdll->runtime_symbols.length,
            0,
            &runtime_symbol
        )) {
        EXIT("couldn't find runtime symbol '%s'\n", runtime_relocation->name);
    }

    *got_entry = runtime_symbol->value;
    LOGTRACE(
        "%x: %s(%x, %x, %x, %x, %x, %x, %x, %x)\n",
        runtime_symbol->value,
        runtime_relocation->name,
        rdi,
        rsi,
        rdx,
        rcx,
        r8,
        r9,
        p7_stack1,
        p8_stack2
    );
    LOGTRACE("Completed dynamic linking to %x\n", runtime_symbol->value);

    __asm__(

        ".dynamic_callback_linux:\n"
        "mov rdi, %[p1_rdi]\n"
        "mov rsi, %[p2_rsi]\n"
        "mov rdx, %[p3_rdx]\n"
        "mov rcx, %[p4_rcx]\n"
        "mov r8, %[p5_r8]\n"
        "mov r9, %[p6_r9]\n"
        "mov r12, %[r12]\n"
        "mov r13, %[r13]\n"
        "mov r14, %[r14]\n"
        "mov r15, %[r15]\n"

        "movsd xmm0, %[xmm0]\n"
        "movsd xmm1, %[xmm1]\n"

        "mov rbx, %[rbx]\n"
        "mov rsp, rbp\n"
        "pop rbp\n"
        "add rsp, 16\n" // remove dynamic linking information
        "jmp %[function_address]\n"
        :
        :

        [function_address] "r"(runtime_symbol->value),
        [p1_rdi] "m"(rdi),
        [p2_rsi] "m"(rsi),
        [p3_rdx] "m"(rdx),
        [p4_rcx] "m"(rcx),
        [p5_r8] "m"(r8),
        [p6_r9] "m"(r9),
        [rbx] "m"(rbx),
        [r12] "m"(r12),
        [r13] "m"(r13),
        [r14] "m"(r14),
        [r15] "m"(r15),
        [xmm0] "m"(xmm0),
        [xmm1] "m"(xmm1)
    );
}

/*
 * Align stack after being called from trampoline function
 */
__attribute__((naked)) void dynamic_callback_windows_pre(void) {
    __asm__(

        "push 0\n"
        "jmp dynamic_callback_windows\n"
    );
}

/*
 * Dynamic callback from Windows to Windows, or from Windows to Linux
 * libntdll.so.
 */
void dynamic_callback_windows(void) {
    size_t rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14, r15, *rbp;
    double xmm0, xmm1;
    GET_REGISTERS_SNAPSHOT();

    size_t dyn_trampoline_end = rbp[2];
    size_t p5_win_stack1 = rbp[8];
    size_t p6_win_stack2 = rbp[9];
    size_t p7_win_stack3 = rbp[10];
    size_t p8_win_stack4 = rbp[11];

    size_t dyn_trampoline_start =
        dyn_trampoline_end - DYNAMIC_CALLBACK_TRAMPOLINE_SIZE;
    size_t runtime_iat_section_base = dyn_trampoline_start /
        MAX_TRAMPOLINE_IAT_SIZE * MAX_TRAMPOLINE_IAT_SIZE;
    size_t func_iat_value_raw =
        dyn_trampoline_start - initial_global_runtime_iat_region_base;

    LOGTRACE("---Starting Windows dyn callback---\n");

    /** Find entry in Import Address Table */

    struct WinRuntimeObject *source_iat_object = NULL;
    size_t func_iat_key = 0;
    size_t func_iat_value = 0;
    if (runtime_iat_section_base == runtime_exe.runtime_iat_section_base) {
        source_iat_object = &runtime_exe;
        func_iat_value = func_iat_value_raw;
        size_t runtime_obj_iat_len =
            runtime_exe.pe_data.import_address_table_len;
        for (size_t i = 0; i < runtime_obj_iat_len; i++) {
            struct ImportAddressEntry *iat_entry =
                &runtime_exe.pe_data.import_address_table[i];
            if (iat_entry->value == func_iat_value) {
                func_iat_key = iat_entry->key;
                break;
            }
        }
    } else {
        for (size_t i = 0; i < shared_libraries.length; i++) {
            WinRuntimeObject *curr_shared_lib = &shared_libraries.data[i];
            if (runtime_iat_section_base !=
                curr_shared_lib->runtime_iat_section_base) {
                continue;
            }

            source_iat_object = curr_shared_lib;
            size_t runtime_iat_offset =
                curr_shared_lib->runtime_iat_section_base -
                curr_shared_lib->pe_data.import_section->virtual_base_address -
                initial_global_runtime_iat_region_base;
            func_iat_value = func_iat_value_raw - runtime_iat_offset;
            size_t runtime_obj_iat_len =
                curr_shared_lib->pe_data.import_address_table_len;
            for (size_t i = 0; i < runtime_obj_iat_len; i++) {
                struct ImportAddressEntry *iat_entry =
                    &curr_shared_lib->pe_data.import_address_table[i];
                if (iat_entry->value == func_iat_value) {
                    func_iat_key = iat_entry->key;
                    break;
                }
            }
            break;
        }
    }
    if (func_iat_key == 0) {
        EXIT("func_iat_value_raw '%x' not found\n", func_iat_value_raw);
    }

    LOGTRACE(
        "%s IAT: %x:%x\n", source_iat_object->name, func_iat_key, func_iat_value
    );

    /** Find import entry using IAT entry */

    const char *lib_name = NULL;
    struct ImportEntry *import_entry = NULL;
    struct PeData *source_iat_pe = &source_iat_object->pe_data;
    for (size_t i = 0; i < source_iat_pe->import_dir_entries_len; i++) {
        struct ImportDirectoryEntry *dir_entry =
            &source_iat_pe->import_dir_entries[i];
        for (size_t i = 0; i < dir_entry->import_entries_len; i++) {
            struct ImportEntry *current_entry = &dir_entry->import_entries[i];
            if (current_entry->address == func_iat_value) {
                lib_name = dir_entry->lib_name;
                import_entry = current_entry;
            }
        }
    }
    if (import_entry == NULL) {
        EXIT(
            "import_entry %x not found in %s IAT\n",
            func_iat_value,
            source_iat_object->name
        );
    };

    LOGTRACE(
        "%s: %s(%x, %x, %x, %x, %x, %x, %x, %x)\n",
        lib_name,
        import_entry->name,
        rcx,
        rdx,
        r8,
        r9,
        p5_win_stack1,
        p6_win_stack2,
        p7_win_stack3,
        p8_win_stack4
    );

    /** Find function using library and function name */

    WinRuntimeExport function_export = {};
    bool is_lib_ntdll;
    if (strcmp(lib_name, "ntdll.dll") == 0) {
        is_lib_ntdll = true;
        for (size_t i = 0; i < lib_ntdll->runtime_symbols.length; i++) {
            RuntimeSymbol *curr_symbol = &lib_ntdll->runtime_symbols.data[i];
            if (strcmp(curr_symbol->name, import_entry->name) == 0) {
                function_export = (WinRuntimeExport){
                    .address = curr_symbol->value,
                    .name = curr_symbol->name,
                };
                break;
            }
        }
    } else {
        is_lib_ntdll = false;
        for (size_t i = 0; i < shared_libraries.length; i++) {
            WinRuntimeObject *shared_lib = &shared_libraries.data[i];
            for (size_t i = 0; i < shared_lib->function_exports.length; i++) {
                WinRuntimeExport *curr_func_export =
                    &shared_lib->function_exports.data[i];
                if (strcmp(curr_func_export->name, import_entry->name) == 0) {
                    function_export = *curr_func_export;
                    break;
                }
            }
        }
    }
    if (function_export.address == 0) {
        EXIT("expected runtime function\n");
    }

    LOGTRACE("Completed dynamic linking to %x\n", function_export.address);

    if (is_lib_ntdll) {
        /* Converts from Windows state to Linux state, and back */

        preserved_swap_state = (struct PreservedSwapState){
            .rbx = rbx,
            .rdi = rdi,
            .rsi = rsi,
        };
        __asm__(
            /* Setup registers */

            "mov rdi, %[p1_win_rcx]\n"
            "mov rsi, %[p2_win_rdx]\n"
            "mov rdx, %[p3_win_r8]\n"
            "mov rcx, %[p4_win_r9]\n"
            "mov r8, %[p5_win_stack1]\n"
            "mov r9, %[p6_win_stack2]\n"
            "mov r12, %[r12]\n"
            "mov r13, %[r13]\n"
            "mov r14, %[r14]\n"
            "mov r15, %[r15]\n"

            "movsd xmm0, %[xmm0]\n"
            "movsd xmm1, %[xmm1]\n"

            ".loader_dynamic_callback_windows_swap:\n"

            "mov rsp, rbp\n"
            "pop rbp\n"
            "add rsp, 16\n" // Remove dynamic trampoline data

            /* Convert to linux stack frame */

            "pop rbx\n"     // Save return address
            "add rsp, 32\n" // Remove frame padding
            "add rsp, 16\n" // Remove stack params 5 & 6
            // "add rsp, 8\n"  // Convert padding
            "call %[function_address]\n"

            /* Restore windows stack frame and remaining registers */

            // "sub rsp, 8\n"  // Convert padding
            "sub rsp, 16\n" // Restore stack params 5 & 6 space
            "sub rsp, 32\n" // Restore frame padding
            "push rbx\n"    // Restore return address
            "mov rbx, [%[swap_state_rbx]]\n"
            "mov rdi, [%[swap_state_rdi]]\n"
            "mov rsi, [%[swap_state_rsi]]\n"
            "ret\n"
            :
            : [function_address] "r"(function_export.address),
              [p1_win_rcx] "m"(rcx),
              [p2_win_rdx] "m"(rdx),
              [p3_win_r8] "m"(r8),
              [p4_win_r9] "m"(r9),
              [p5_win_stack1] "m"(p5_win_stack1),
              [p6_win_stack2] "m"(p6_win_stack2),
              [r12] "m"(r12),
              [r13] "m"(r13),
              [r14] "m"(r14),
              [r15] "m"(r15),
              [swap_state_rbx] "g"(&preserved_swap_state.rbx),
              [swap_state_rdi] "g"(&preserved_swap_state.rdi),
              [swap_state_rsi] "g"(&preserved_swap_state.rsi),
              [xmm0] "m"(xmm0),
              [xmm1] "m"(xmm1)
            :
        );
    } else {
        __asm__(
            /* */
            "mov rcx, %[p1_win_rcx]\n"
            "mov rdx, %[p2_win_rdx]\n"
            "mov r8, %[p3_win_r8]\n"
            "mov r9, %[p4_win_r9]\n"
            "mov r12, %[r12]\n"
            "mov r13, %[r13]\n"
            "mov r14, %[r14]\n"
            "mov r15, %[r15]\n"
            "mov rbx, %[rbx]\n"
            "mov rdi, %[rdi]\n"
            "mov rsi, %[rsi]\n"

            "movsd xmm0, %[xmm0]\n"
            "movsd xmm1, %[xmm1]\n"

            ".loader_dynamic_callback_windows:\n"

            "mov rsp, rbp\n"
            "pop rbp\n"
            "add rsp, 16\n" // Remove dynamic trampoline data
            "jmp %[function_address]\n"
            :
            : [function_address] "r"(function_export.address),
              [p1_win_rcx] "m"(rcx),
              [p2_win_rdx] "m"(rdx),
              [p3_win_r8] "m"(r8),
              [p4_win_r9] "m"(r9),
              [r12] "m"(r12),
              [r13] "m"(r13),
              [r14] "m"(r14),
              [r15] "m"(r15),
              [rbx] "m"(rbx),
              [rdi] "m"(rdi),
              [rsi] "m"(rsi),
              [xmm0] "m"(xmm0),
              [xmm1] "m"(xmm1)
            :
        );
    }
}

static bool initialize_lib_ntdll(struct RuntimeObject *lib_ntdll_object) {
    const char *LIB_NTDLL_SO_NAME = "libntdll.so";

    int32_t ntdll_file = open(LIB_NTDLL_SO_NAME, O_RDONLY);
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

    MemoryRegionList memory_regions = {
        .allocator = loader_malloc_arena,
    };
    if (!get_memory_regions(
            ntdll_elf.program_headers, ntdll_elf.header.e_phnum, &memory_regions
        )) {
        BAIL("failed getting memory regions\n");
    }

    size_t reserved_address;
    if (!reserve_region_space(&memory_regions, &reserved_address)) {
        BAIL("get_reserved_region_space failed\n");
    }

    LOGINFO("Mapping shared library 'lib_ntdll.so'\n");
    if (!map_memory_regions(
            ntdll_file, memory_regions.data, memory_regions.length
        )) {
        BAIL("loader lib map memory regions failed\n");
    }

    close(ntdll_file);

    uint8_t *bss = 0;
    size_t bss_len = 0;
    const struct SectionHeader *bss_section_header = find_section_header(
        ntdll_elf.section_headers, ntdll_elf.section_headers_len, ".bss"
    );
    if (bss_section_header != NULL) {
        bss = (uint8_t *)(reserved_address + bss_section_header->addr);
        bss_len = bss_section_header->size;
    }

    struct RuntimeRelocation *runtime_func_relocations;
    size_t runtime_func_relocations_len;
    if (!get_function_relocations(
            ntdll_elf.dynamic_data,
            reserved_address,
            &runtime_func_relocations,
            &runtime_func_relocations_len
        )) {
        BAIL("get_function_relocations failed\n");
    }

    RuntimeSymbolList runtime_symbols = {
        .allocator = loader_malloc_arena,
    };
    if (!find_symbols(
            ntdll_elf.dynamic_data, reserved_address, &runtime_symbols
        )) {
        BAIL("failed getting symbols\n");
    }

    RuntimeGotEntryList runtime_got_entries = {
        .allocator = loader_malloc_arena,
    };
    if (!get_runtime_got(
            ntdll_elf.dynamic_data,
            reserved_address,
            (size_t)dynamic_callback_linux,
            got_lib_dyn_offset_table + 1,
            &runtime_got_entries
        )) {
        BAIL("get_runtime_got failed\n");
    }

    /** Init GOT */

    LOGINFO("GOT entries: %d\n", runtime_got_entries.length);
    for (size_t i = 0; i < runtime_got_entries.length; i++) {
        struct RuntimeGotEntry *runtime_got_entry =
            &runtime_got_entries.data[i];
        LOGDEBUG(
            "GOT entry %d: %x == %x, variable: %s\n",
            i + 1,
            runtime_got_entry->index,
            runtime_got_entry->value
        );
        size_t *got_pointer = (size_t *)runtime_got_entry->index;
        *got_pointer = runtime_got_entry->value;

        if (runtime_got_entry->lib_dynamic_offset > 0) {
            size_t *lib_dyn_offset_table = (size_t *)runtime_got_entry->value;
            *lib_dyn_offset_table = runtime_got_entry->lib_dynamic_offset;
        }
    }

    *lib_ntdll_object = (struct RuntimeObject){
        .name = LIB_NTDLL_SO_NAME,
        .dynamic_offset = reserved_address,
        .elf_data = ntdll_elf,
        .memory_regions = memory_regions,
        .runtime_func_relocations = runtime_func_relocations,
        .runtime_func_relocations_len = runtime_func_relocations_len,
        .bss = bss,
        .bss_len = bss_len,
        .runtime_symbols = runtime_symbols,

    };

    return true;
}

static bool load_dlls(
    struct PeData *inferior_executable,
    WinRuntimeObjectList *shared_libraries,
    size_t curr_global_runtime_iat_base
) {
    for (size_t i = 0; i < inferior_executable->import_dir_entries_len; i++) {
        /* Map shared libraries */

        struct ImportDirectoryEntry *dir_entry =
            &inferior_executable->import_dir_entries[i];
        const char *shared_lib_name = dir_entry->lib_name;
        LOGINFO("mapping shared library '%s'\n", shared_lib_name);
        int32_t shared_lib_file = open(shared_lib_name, O_RDONLY);
        if (shared_lib_file == -1) {
            BAIL("failed opening shared lib '%s'\n", shared_lib_name);
        }

        struct PeData shared_lib_pe;
        if (!get_pe_data(shared_lib_file, &shared_lib_pe)) {
            BAIL("failed getting data for shared lib '%s'\n", shared_lib_name);
        }

        size_t shared_lib_image_base =
            shared_lib_pe.winpe_header->image_optional_header.image_base;
        MemoryRegionList memory_regions = (MemoryRegionList){
            .allocator = loader_malloc_arena,
        };
        if (!get_memory_regions_win(
                shared_lib_pe.section_headers,
                shared_lib_pe.section_headers_len,
                shared_lib_image_base,
                &memory_regions
            )) {
            BAIL("failed getting memory regions\n");
        }

        if (!map_memory_regions_win(
                shared_lib_file, memory_regions.data, memory_regions.length
            )) {
            BAIL("loader lib map memory regions failed\n");
        }

        close(shared_lib_file);
        if (!log_memory_regions()) {
            BAIL("print_memory_regions failed\n");
        }

        /* Init .bss */

        const struct WinSectionHeader *bss_header = find_win_section_header(
            shared_lib_pe.section_headers,
            shared_lib_pe.section_headers_len,
            ".bss"
        );
        if (bss_header != NULL) {
            uint8_t *bss_region = (uint8_t *)(shared_lib_image_base +
                                              bss_header->virtual_base_address);
            memset(bss_region, 0, bss_header->virtual_size);
        }

        /* Load runtime exports */

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
                if (strcmp(import_entry->name, curr_export_entry->name) == 0) {
                    export_entry = curr_export_entry;
                }
            }
            if (export_entry == NULL) {
                BAIL("import '%s' not found\n", import_entry->name);
            }

            struct WinRuntimeExport runtime_relocation = {
                .address = shared_lib_image_base + export_entry->address,
                .name = export_entry->name,
            };
            WinRuntimeExportList_add(&runtime_exports, runtime_relocation);
        }

        /* Get IAT offsets */

        size_t runtime_iat_region_base = 0;
        size_t runtime_iat_section_base = 0;
        if (shared_lib_pe.import_address_table_len) {
            runtime_iat_region_base = curr_global_runtime_iat_base;
            runtime_iat_section_base = runtime_iat_region_base +
                shared_lib_pe.import_section->virtual_base_address;

            curr_global_runtime_iat_base += MAX_TRAMPOLINE_IAT_SIZE;
        }

        struct WinRuntimeObject shared_lib = {
            .name = dir_entry->lib_name,
            .pe_data = shared_lib_pe,
            .memory_regions = memory_regions,
            .function_exports = runtime_exports,
            .runtime_iat_section_base = runtime_iat_section_base,
        };
        WinRuntimeObjectList_add(shared_libraries, shared_lib);
    }

    return true;
}

static bool initialize_import_address_table(
    const struct WinRuntimeObject *runtime_obj
) {
    if (!runtime_obj->pe_data.import_address_table_len) {
        return true;
    }

    RuntimeImportAddressEntryList runtime_import_table = {
        .allocator = loader_malloc_arena,
    };
    size_t runtime_iat_region_base = runtime_obj->runtime_iat_section_base -
        runtime_obj->pe_data.import_section->virtual_base_address;
    if (!get_runtime_import_address_table(
            runtime_obj->pe_data.import_address_table,
            runtime_obj->pe_data.import_address_table_len,
            &shared_libraries,
            &runtime_import_table,
            runtime_obj->pe_data.winpe_header->image_optional_header.image_base,
            runtime_iat_region_base
        )) {
        BAIL("get_runtime_import_address_table failed\n");
    }

    if (!map_import_address_table(
            &runtime_import_table,
            (size_t)dynamic_callback_windows_pre,
            runtime_obj->runtime_iat_section_base
        )) {
        BAIL("map_import_address_table failed\n");
    }

    return true;
}

__attribute__((naked)) void win_loader_implicit_end(void) {
    __asm__(

        "mov rdi, rax\n"
        "mov rax, 0x3c\n"
        "syscall\n"
    );
}

int main(int argc, char **argv) {
    if (argc < 2) {
        EXIT("Filename required\n", argc);
    }

    char *filename = argv[1];
    LOGINFO("Starting winloader, %s, %d\n", filename, argc);

    /* Init heap */

    size_t brk_start = tinyc_sys_brk(0);
    LOGINFO("BRK:, %x\n", brk_start);
    size_t brk_end = tinyc_sys_brk(brk_start + 0x1000);
    LOGINFO("BRK:, %x\n", brk_end);
    if (brk_end <= brk_start) {
        EXIT("program BRK setup failed");
    }

    int32_t pid = getpid();
    LOGINFO("pid: %d\n", pid);

    /* Unmap default locations */

    if (munmap((void *)0x10000, 0x1000)) {
        EXIT("munmap of self failed\n");
    }
    if (munmap((void *)0x400000, 0x1000)) {
        EXIT("munmap of self failed\n");
        return -1;
    }

    int32_t fd = open(filename, O_RDONLY);
    if (fd < 0) {
        EXIT("file error, %d, %s\n", errno, strerror(errno));
        return -1;
    }

    struct PeData pe_exe;
    if (!get_pe_data(fd, &pe_exe)) {
        EXIT("error parsing pe data\n");
    }

    size_t image_base = pe_exe.winpe_header->image_optional_header.image_base;

    MemoryRegionList memory_regions = (MemoryRegionList){
        .allocator = loader_malloc_arena,
    };
    size_t win_headers_size = sizeof(struct ImageDosHeader) +
        sizeof(struct WinPEHeader) + pe_exe.winpe_optional_header.headers_size;
    if (win_headers_size > 0x1000) {
        EXIT("Unsupported win_headers_size\n");
    }

    MemoryRegionList_add(
        &memory_regions,
        (struct MemoryRegion){
            .start = image_base,
            .end = image_base + win_headers_size,
            .is_direct_file_map = false,
            .file_offset = 0x00,
            .file_size = win_headers_size,
            .permissions = 4 | 0 | 0,
        }
    );
    if (!get_memory_regions_win(
            pe_exe.section_headers,
            pe_exe.section_headers_len,
            image_base,
            &memory_regions
        )) {
        EXIT("failed getting memory regions\n");
    }

    LOGINFO("Mapping executable memory\n");
    if (!map_memory_regions_win(
            fd, memory_regions.data, memory_regions.length
        )) {
        EXIT("map_memory_regions_win failed\n");
    }

    /* Init .bss */

    const struct WinSectionHeader *bss_header = find_win_section_header(
        pe_exe.section_headers, pe_exe.section_headers_len, ".bss"
    );
    if (bss_header != NULL) {
        uint8_t *bss_region =
            (uint8_t *)(image_base + bss_header->virtual_base_address);
        memset(bss_region, 0, bss_header->virtual_size);
    }

    /* Load libntdll.so */

    lib_ntdll = loader_malloc_arena(sizeof(struct RuntimeObject));
    if (!initialize_lib_ntdll(lib_ntdll)) {
        EXIT("initialize_lib_ntdll failed\n");
    }

    /* Get IAT offsets */

    const size_t TRAMPOLINE_IAT_RESERVED_SIZE = MAX_TRAMPOLINE_IAT_SIZE * 50;
    initial_global_runtime_iat_region_base = (size_t)mmap(
        NULL,
        TRAMPOLINE_IAT_RESERVED_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (initial_global_runtime_iat_region_base == (size_t)MAP_FAILED) {
        EXIT("initial_global_runtime_iat_base memory regions failed\n");
    }

    size_t runtime_iat_section_base = 0;
    if (pe_exe.import_address_table_len) {
        runtime_iat_section_base = initial_global_runtime_iat_region_base +
            pe_exe.import_section->virtual_base_address;
    }

    /* Load dlls */

    shared_libraries = (WinRuntimeObjectList){
        .allocator = loader_malloc_arena,
    };
    if (!load_dlls(
            &pe_exe,
            &shared_libraries,
            initial_global_runtime_iat_region_base + MAX_TRAMPOLINE_IAT_SIZE
        )) {
        EXIT("initialize_dynamic_data failed\n");
    }

    runtime_exe = (struct WinRuntimeObject){
        .name = filename,
        .pe_data = pe_exe,
        .memory_regions = memory_regions,
        .function_exports = {},
        .runtime_iat_section_base = runtime_iat_section_base,
    };

    /* Map Import Address Tables */

    if (munmap(
            (void *)initial_global_runtime_iat_region_base,
            TRAMPOLINE_IAT_RESERVED_SIZE
        )) {
        EXIT("munmap initial_global_runtime_iat_base failed\n");
    }

    LOGINFO("Initializing executable IAT\n");
    if (!initialize_import_address_table(&runtime_exe)) {
        log_memory_regions();
        EXIT("initialize_import_address_table failed\n");
    }
    for (size_t i = 0; i < shared_libraries.length; i++) {
        const struct WinRuntimeObject *shared_library =
            &shared_libraries.data[i];
        LOGINFO("Initializing '%s' IAT\n", shared_library->name);
        if (!initialize_import_address_table(shared_library)) {
            log_memory_regions();
            EXIT("initialize_import_address_table failed\n");
        }
    }

    log_memory_regions();

    /* Thread Local Storage and process params init */

    size_t *gs_mem = loader_malloc_arena(0x100);
    if (gs_mem == NULL) {
        EXIT("gs_region memory init failed\n");
    }
    if (tinyc_sys_arch_prctl(ARCH_SET_GS, (size_t)gs_mem) != 0) {
        EXIT("tinyc_sys_arch_prctl failed\n");
    }
    USHORT CMD_MAX_LEN = 0x100;
    WCHAR *cmd = loader_malloc_arena(sizeof(WCHAR) * CMD_MAX_LEN);
    if (cmd == NULL) {
        EXIT("cmd memory init failed\n");
    }
    RTL_USER_PROCESS_PARAMETERS *params =
        loader_malloc_arena(sizeof(RTL_USER_PROCESS_PARAMETERS));
    if (params == NULL) {
        EXIT("params memory init failed\n");
    }
    PEB *peb = loader_malloc_arena(sizeof(PEB));
    if (peb == NULL) {
        EXIT("peb memory init failed\n");
    }
    TEB *teb = loader_malloc_arena(sizeof(TEB));
    if (teb == NULL) {
        EXIT("teb memory init failed\n");
    }

    size_t cmd_len = 0;
    size_t inferior_argc = (size_t)argc - 1;
    for (size_t i = 0; i < inferior_argc; i++) {
        char *string = argv[i + 1];
        size_t string_len = strlen(string);
        LOGINFO("stack arg %d: %s\n", i, string);
        for (size_t i = 0; i < string_len; i++) {
            cmd[cmd_len] = (WCHAR)string[i];
            cmd_len += 1;
        }
        if (i + 1 != inferior_argc) {
            cmd[cmd_len] = ' ';
            cmd_len += 1;
        }
    }

    if (cmd_len >= CMD_MAX_LEN) {
        EXIT("Command line length exceeded max %d\n", CMD_MAX_LEN);
    }

    *params = (RTL_USER_PROCESS_PARAMETERS){
        .CommandLine =
            {
                .Length = (USHORT)cmd_len,
                .MaximumLength = CMD_MAX_LEN,
                .Buffer = cmd,
            },
    };
    *peb = (PEB){
        .ProcessParameters = params,
    };
    *teb = (TEB){
        .ProcessEnvironmentBlock = peb,
    };

    gs_mem[0x30 / sizeof(uint64_t)] = (size_t)teb;
    gs_mem[0x60 / sizeof(uint64_t)] = (size_t)peb;

    /* Jump to program */

    size_t *inferior_stack = (size_t *)argv;
    if ((size_t)inferior_stack % 16 != 8) {
        EXIT("Stack must NOT be 16 byte aligned when leaving winloader");
    }
    *inferior_stack = (size_t)win_loader_implicit_end;

    LOGINFO("inferior_entry: %zx\n", pe_exe.entrypoint);
    LOGINFO("inferior_stack: %zx\n", inferior_stack);
    LOGINFO("------------running program------------\n");

    __asm__(

        ".loader_jump_to_entry:\n"
        "mov rsp, %[inferior_stack]\n"
        "jmp %[inferior_entry]\n"
        :
        :

        [inferior_stack] "m"(inferior_stack),
        [inferior_entry] "m"(pe_exe.entrypoint)
    );
}
