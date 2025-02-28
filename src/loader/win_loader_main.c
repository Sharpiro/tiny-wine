#include "../tiny_c/tiny_c.h"
#include "../tiny_c/tinyc_sys.h"
#include "./pe_tools.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include "win_loader_lib.h"
#include <asm/prctl.h>
#include <fcntl.h>
#include <linux/prctl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

#define IAT_INCREMENT 0x10000

size_t rdi = 0;
size_t rsi = 0;
struct WinRuntimeObject runtime_exe;
struct RuntimeObject *lib_ntdll;
WinRuntimeObjectList shared_libraries = {};
size_t initial_global_runtime_iat_base = 0;
size_t curr_global_runtime_iat_base = 0;
size_t curr_global_runtime_iat_offset = 0;
size_t got_lib_dyn_offset_table[100] = {};

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

    __asm__("mov rbp, 0x00\n"
            "jmp %[program_entry]\n"
            :
            : [program_entry] "r"(program_entry)
            : "rax");
}

/*
 * Dynamic callback from Linux to Linux
 */
static void dynamic_callback_linux(void) {
    size_t *rbx;
    __asm__("mov %0, rbx" : "=r"(rbx));
    size_t *r12;
    __asm__("mov %0, r12" : "=r"(r12));
    size_t *r13;
    __asm__("mov %0, r13" : "=r"(r13));
    size_t *r14;
    __asm__("mov %0, r14" : "=r"(r14));
    size_t *r15;
    __asm__("mov %0, r15" : "=r"(r15));
    size_t *rbp;
    __asm__("mov %0, rbp" : "=r"(rbp));
    size_t *p1_linux_rdi;
    __asm__("mov %0, rdi" : "=r"(p1_linux_rdi));
    size_t *p2_linux_rsi;
    __asm__("mov %0, rsi" : "=r"(p2_linux_rsi));
    size_t *p3_linux_rdx;
    __asm__("mov %0, rdx" : "=r"(p3_linux_rdx));
    size_t *p4_linux_rcx;
    __asm__("mov %0, rcx" : "=r"(p4_linux_rcx));
    size_t *p5_linux_r8;
    __asm__("mov %0, r8" : "=r"(p5_linux_r8));
    size_t *p6_linux_r9;
    __asm__("mov %0, r9" : "=r"(p6_linux_r9));
    size_t p7_stack1 = *(rbp + 4);
    size_t p8_stack2 = *(rbp + 5);

    LOADER_LOG(
        "--- Starting Linux dyn callback -> at %x\n", dynamic_callback_linux
    );

    size_t *lib_dyn_offset = (size_t *)(*(rbp + 1));
    size_t relocation_index = *(rbp + 2);
    if (lib_dyn_offset == NULL) {
        EXIT("lib_dyn_offset was null\n");
    }

    LOADER_LOG(
        "relocation params: %x, %x\n", *lib_dyn_offset, relocation_index
    );

    struct RuntimeRelocation *runtime_relocation =
        &lib_ntdll->runtime_func_relocations[relocation_index];

    size_t *got_entry = (size_t *)runtime_relocation->offset;
    LOADER_LOG("got_entry: %x: %x\n", got_entry, *got_entry);

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
    LOADER_LOG(
        "%x: %s(%x, %x, %x, %x, %x, %x, %x, %x)\n",
        runtime_symbol->value,
        runtime_relocation->name,
        p1_linux_rdi,
        p2_linux_rsi,
        p3_linux_rdx,
        p4_linux_rcx,
        p5_linux_r8,
        p6_linux_r9,
        p7_stack1,
        p8_stack2
    );
    LOADER_LOG("rbx: %x, rdi: %x, rsi: %x\n", rbx, rdi, rsi);
    LOADER_LOG("--- Completed dynamic linking\n");

    __asm__(
        /* */
        ".dynamic_callback_linux:\n"
        "mov rdi, %[p1_linux_rdi]\n"
        "mov rsi, %[p2_linux_rsi]\n"
        "mov rdx, %[p3_linux_rdx]\n"
        "mov rcx, %[p4_linux_rcx]\n"
        "mov r8, %[p5_linux_r8]\n"
        "mov r9, %[p6_linux_r9]\n"
        "mov r12, %[r12]\n"
        "mov r13, %[r13]\n"
        "mov r14, %[r14]\n"
        "mov r15, %[r15]\n"

        "mov rbx, %[rbx]\n"
        "mov rsp, rbp\n"
        "pop rbp\n"
        "add rsp, 16\n"
        "jmp %[function_address]\n"
        :
        : [function_address] "m"(runtime_symbol->value),
          [p1_linux_rdi] "m"(p1_linux_rdi),
          [p2_linux_rsi] "m"(p2_linux_rsi),
          [p3_linux_rdx] "m"(p3_linux_rdx),
          [p4_linux_rcx] "m"(p4_linux_rcx),
          [p5_linux_r8] "m"(p5_linux_r8),
          [p6_linux_r9] "m"(p6_linux_r9),
          [rbx] "m"(rbx),
          [r12] "m"(r12),
          [r13] "m"(r13),
          [r14] "m"(r14),
          [r15] "m"(r15)
    );
}

/*
 * Dynamic callback from Windows to Windows, or from Windows to Linux
 * libntdll.so
 */
static void dynamic_callback_windows(void) {
    size_t *rbx;
    __asm__("mov %0, rbx" : "=r"(rbx));
    __asm__("mov %0, rdi" : "=r"(rdi));
    __asm__("mov %0, rsi" : "=r"(rsi));
    size_t *r12;
    __asm__("mov %0, r12" : "=r"(r12));
    size_t *r13;
    __asm__("mov %0, r13" : "=r"(r13));
    size_t *r14;
    __asm__("mov %0, r14" : "=r"(r14));
    size_t *r15;
    __asm__("mov %0, r15" : "=r"(r15));
    size_t *rbp;
    __asm__("mov %0, rbp" : "=r"(rbp));
    size_t p1_win_rcx;
    __asm__("mov %0, rcx" : "=r"(p1_win_rcx));
    size_t p2_win_rdx;
    __asm__("mov %0, rdx" : "=r"(p2_win_rdx));
    size_t p3_win_r8;
    __asm__("mov %0, r8" : "=r"(p3_win_r8));
    size_t p4_win_r9;
    __asm__("mov %0, r9" : "=r"(p4_win_r9));
    size_t p5_stack1 = rbp[7];
    size_t p6_stack2 = rbp[8];
    size_t p7_stack3 = rbp[9];
    size_t p8_stack4 = rbp[10];

    size_t dyn_trampoline_end = *(rbp + 1);
    size_t dyn_trampoline_start =
        dyn_trampoline_end - DYNAMIC_CALLBACK_TRAMPOLINE_SIZE;
    size_t iat_runtime_base = dyn_trampoline_start / 0x1000 * 0x1000;
    size_t func_iat_value_temp =
        dyn_trampoline_start - initial_global_runtime_iat_base;

    LOADER_LOG(
        "--- Starting Windows dyn callback -> ??? at %x\n",
        dynamic_callback_windows
    );

    /** Find entry in Import Address Table */

    struct PeData *source_pe = NULL;
    size_t func_iat_key = 0;
    size_t func_iat_value = 0;
    if (iat_runtime_base == runtime_exe.runtime_iat_section_base) {
        source_pe = &runtime_exe.pe_data;
        for (size_t i = 0; i < runtime_exe.pe_data.import_address_table_len;
             i++) {
            struct ImportAddressEntry *iat_entry =
                &runtime_exe.pe_data.import_address_table[i];
            size_t curr_func_iat_value =
                func_iat_value_temp - runtime_exe.runtime_iat_offset;
            if (iat_entry->value == curr_func_iat_value) {
                func_iat_key = iat_entry->key;
                func_iat_value = iat_entry->value;
                break;
            }
        }
    } else {
        for (size_t i = 0; i < shared_libraries.length; i++) {
            WinRuntimeObject *curr_shared_lib = &shared_libraries.data[i];
            if (iat_runtime_base != curr_shared_lib->runtime_iat_section_base) {
                continue;
            }

            source_pe = &curr_shared_lib->pe_data;
            for (size_t i = 0;
                 i < curr_shared_lib->pe_data.import_address_table_len;
                 i++) {
                struct ImportAddressEntry *iat_entry =
                    &curr_shared_lib->pe_data.import_address_table[i];
                size_t curr_func_iat_value =
                    func_iat_value_temp - curr_shared_lib->runtime_iat_offset;
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

    /** Find import entry using IAT entry */

    const char *lib_name = NULL;
    struct ImportEntry *import_entry = NULL;
    for (size_t i = 0; i < source_pe->import_dir_entries_len; i++) {
        struct ImportDirectoryEntry *dir_entry =
            &source_pe->import_dir_entries[i];
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
        "%s: %s(%x, %x, %x, %x, %x, %x, %x, %x)\n",
        lib_name,
        import_entry->name,
        p1_win_rcx,
        p2_win_rdx,
        p3_win_r8,
        p4_win_r9,
        p5_stack1,
        p6_stack2,
        p7_stack3,
        p8_stack4
    );
    LOADER_LOG("rbx: %x, rdi: %x, rsi: %x\n", rbx, rdi, rsi);

    /** Find function using library and function name */

    WinRuntimeExport function_export = {};
    bool is_lib_ntdll;
    if (tiny_c_strcmp(lib_name, "ntdll.dll") == 0) {
        is_lib_ntdll = true;
        LOADER_LOG("Rerouting to libntdll.so\n");
        for (size_t i = 0; i < lib_ntdll->runtime_symbols.length; i++) {
            RuntimeSymbol *curr_symbol = &lib_ntdll->runtime_symbols.data[i];
            if (tiny_c_strcmp(curr_symbol->name, import_entry->name) == 0) {
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
                if (tiny_c_strcmp(curr_func_export->name, import_entry->name) ==
                    0) {
                    function_export = *curr_func_export;
                    break;
                }
            }
        }
    }
    if (function_export.address == 0) {
        EXIT("expected runtime function\n");
    }

    LOADER_LOG("runtime function %x\n", function_export.address);

    LOADER_LOG("Completed dynamic linking\n");

    if (is_lib_ntdll) {
        /* Converts from Windows state to Linux state, and back */
        __asm__(
            /* Setup registers */

            ".dynamic_callback_windows_swap:\n"
            "mov rdi, %[p1_win_rcx]\n"
            "mov rsi, %[p2_win_rdx]\n"
            "mov rdx, %[p3_win_r8]\n"
            "mov rcx, %[p4_win_r9]\n"
            "mov r8, %[p5_stack1]\n"
            "mov r9, %[p6_stack1]\n"
            "mov r12, %[r12]\n"
            "mov r13, %[r13]\n"
            "mov r14, %[r14]\n"
            "mov r15, %[r15]\n"
            "mov rbx, %[rbx]\n"
            "mov rsp, rbp\n"
            "pop rbp\n"
            "add rsp, 8\n"

            /* Duplicate windows stack frame */

            "lea r11, [rsp + 64]\n"
            "lea r10, [rsp]\n"
            ".dynamic_callback_windows_loop_start:\n"
            "cmp r11, r10\n"
            "jl .dynamic_callback_windows_loop_end\n"
            "push [r11]\n"
            "sub r11, 8\n"
            "jmp .dynamic_callback_windows_loop_start\n"
            ".dynamic_callback_windows_loop_end:\n"

            /* Convert to linux stack frame */

            "add rsp, 7 * 8\n"
            "call %[function_address]\n"

            /* Restore original windows state */

            "mov rdi, [%[rdi_pointer]]\n"
            "mov rsi, [%[rsi_pointer]]\n"

            "add rsp, 2 * 8\n"
            "ret\n"
            :
            : [function_address] "m"(function_export.address),
              [p1_win_rcx] "m"(p1_win_rcx),
              [p2_win_rdx] "m"(p2_win_rdx),
              [p3_win_r8] "m"(p3_win_r8),
              [p4_win_r9] "m"(p4_win_r9),
              [p5_stack1] "m"(p5_stack1),
              [p6_stack1] "m"(p6_stack2),
              [r12] "m"(r12),
              [r13] "m"(r13),
              [r14] "m"(r14),
              [r15] "m"(r15),
              [rbx] "m"(rbx),
              [rdi_pointer] "g"(&rdi),
              [rsi_pointer] "g"(&rsi)
            :
        );
    } else {
        __asm__(
            /* */
            ".dynamic_callback_windows:\n"
            "mov rcx, %[p1]\n"
            "mov rdx, %[p2]\n"
            "mov r8, %[p3]\n"
            "mov r9, %[p4]\n"
            "mov r12, %[r12]\n"
            "mov r13, %[r13]\n"
            "mov r14, %[r14]\n"
            "mov r15, %[r15]\n"
            "mov rbx, %[rbx]\n"
            "mov rdi, [%[rdi_pointer]]\n"
            "mov rsi, [%[rsi_pointer]]\n"
            "mov rsp, rbp\n"
            "pop rbp\n"
            "add rsp, 8\n"
            "jmp %[function_address]\n"
            :
            : [function_address] "m"(function_export.address),
              [p1] "r"(p1_win_rcx),
              [p2] "m"(p2_win_rdx),
              [p3] "m"(p3_win_r8),
              [p4] "m"(p4_win_r9),
              [r12] "m"(r12),
              [r13] "m"(r13),
              [r14] "m"(r14),
              [r15] "m"(r15),
              [rbx] "m"(rbx),
              [rdi_pointer] "g"(&rdi),
              [rsi_pointer] "g"(&rsi)
            :
        );
    }
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

    LOADER_LOG("Mapping shared library 'lib_ntdll.so'\n");
    if (!map_memory_regions(
            ntdll_file,
            memory_regions_info.regions,
            memory_regions_info.regions_len
        )) {
        BAIL("loader lib map memory regions failed\n");
    }

    tiny_c_close(ntdll_file);

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
    if (!find_win_symbols(
            ntdll_elf.dynamic_data, dynamic_lib_offset, &runtime_symbols
        )) {
        BAIL("failed getting symbols\n");
    }

    RuntimeGotEntryList runtime_got_entries = {
        .allocator = loader_malloc_arena,
    };
    if (!get_runtime_got(
            ntdll_elf.dynamic_data,
            dynamic_lib_offset,
            (size_t)dynamic_callback_linux,
            got_lib_dyn_offset_table + 1,
            &runtime_got_entries
        )) {
        BAIL("get_runtime_got failed\n");
    }

    /** Init GOT */

    LOADER_LOG("GOT entries: %d\n", runtime_got_entries.length);
    for (size_t i = 0; i < runtime_got_entries.length; i++) {
        struct RuntimeGotEntry *runtime_got_entry =
            &runtime_got_entries.data[i];
        LOADER_LOG(
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
        .dynamic_offset = dynamic_lib_offset,
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

// @todo: recursive dll loading not supported
static bool load_dlls(
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

        size_t shared_lib_image_base =
            shared_lib_pe.winpe_header->image_optional_header.image_base;
        MemoryRegionList memory_regions = (MemoryRegionList){
            .allocator = loader_malloc_arena,
        };
        if (!get_memory_regions_info_win(
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

        tiny_c_close(shared_lib_file);
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
                if (tiny_c_strcmp(
                        import_entry->name, curr_export_entry->name
                    ) == 0) {
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
        size_t runtime_iat_base = 0;
        size_t runtime_iat_offset = 0;
        if (shared_lib_pe.import_address_table_len) {
            const struct WinSectionHeader *idata_header =
                find_win_section_header(
                    shared_lib_pe.section_headers,
                    shared_lib_pe.section_headers_len,
                    ".idata"
                );
            runtime_iat_region_base = curr_global_runtime_iat_base;
            runtime_iat_base =
                runtime_iat_region_base + idata_header->virtual_base_address;

            runtime_iat_offset = curr_global_runtime_iat_offset;
            curr_global_runtime_iat_base += IAT_INCREMENT;
            curr_global_runtime_iat_offset += IAT_INCREMENT;
        }

        struct WinRuntimeObject shared_lib = {
            .name = dir_entry->lib_name,
            .pe_data = shared_lib_pe,
            .memory_regions = memory_regions,
            .function_exports = runtime_exports,
            .runtime_iat_object_base = runtime_iat_region_base,
            .runtime_iat_section_base = runtime_iat_base,
            .runtime_iat_offset = runtime_iat_offset,
        };
        WinRuntimeObjectList_add(shared_libraries, shared_lib);
    }

    return true;
}

static bool initialize_import_address_table(
    const struct WinRuntimeObject *shared_lib
) {
    if (!shared_lib->pe_data.import_address_table_len) {
        return true;
    }

    RuntimeImportAddressEntryList runtime_import_table = {
        .allocator = loader_malloc_arena,
    };
    if (!get_runtime_import_address_table(
            shared_lib->pe_data.import_address_table,
            shared_lib->pe_data.import_address_table_len,
            &shared_libraries,
            &runtime_import_table,
            shared_lib->pe_data.winpe_header->image_optional_header.image_base,
            shared_lib->runtime_iat_object_base
        )) {
        BAIL("get_runtime_import_address_table failed\n");
    }

    if (!map_import_address_table(
            &runtime_import_table,
            (size_t)dynamic_callback_windows,
            shared_lib->runtime_iat_section_base
        )) {
        BAIL("map_import_address_table failed\n");
    }

    return true;
}

__attribute__((naked)) void win_loader_implicit_end(void) {
    __asm__("mov rdi, rax\n"
            "mov rax, 0x3c\n"
            "syscall\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        return -1;
    }

    char *filename = argv[1];
    LOADER_LOG("Starting winloader, %s, %d\n", filename, argc);

    /* Init heap */

    size_t brk_start = tinyc_sys_brk(0);
    LOADER_LOG("BRK:, %x\n", brk_start);
    size_t brk_end = tinyc_sys_brk(brk_start + 0x1000);
    LOADER_LOG("BRK:, %x\n", brk_end);
    if (brk_end <= brk_start) {
        EXIT("program BRK setup failed");
    }

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

    MemoryRegionList memory_regions = (MemoryRegionList){
        .allocator = loader_malloc_arena,
    };
    const size_t WINDOWS_HEADER_SIZE =
        sizeof(struct ImageDosHeader) + sizeof(struct WinPEHeader);

    MemoryRegionList_add(
        &memory_regions,
        (struct MemoryRegion){
            .start = image_base,
            .end = image_base + 0x1000,
            .is_direct_file_map = false,
            .file_offset = 0x00,
            .file_size = WINDOWS_HEADER_SIZE,
            .permissions = 4 | 0 | 0,
        }
    );
    if (!get_memory_regions_info_win(
            pe_exe.section_headers,
            pe_exe.section_headers_len,
            image_base,
            &memory_regions
        )) {
        EXIT("failed getting memory regions\n");
    }

    LOADER_LOG("Mapping executable memory\n");
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

    const size_t IAT_LEN = 0x20000;
    initial_global_runtime_iat_base = (size_t)tiny_c_mmap(
        0, IAT_LEN, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    );
    curr_global_runtime_iat_base = initial_global_runtime_iat_base;
    if (initial_global_runtime_iat_base == (size_t)MAP_FAILED) {
        EXIT("initial_global_runtime_iat_base memory regions failed\n");
    }

    size_t runtime_iat_region_base = 0;
    size_t runtime_iat_base = 0;
    size_t runtime_iat_offset = 0;
    if (pe_exe.import_address_table_len) {
        const struct WinSectionHeader *idata_header = find_win_section_header(
            pe_exe.section_headers, pe_exe.section_headers_len, ".idata"
        );
        runtime_iat_region_base = curr_global_runtime_iat_base;
        runtime_iat_base =
            runtime_iat_region_base + idata_header->virtual_base_address;

        runtime_iat_offset = curr_global_runtime_iat_offset;
        curr_global_runtime_iat_base += IAT_INCREMENT;
        curr_global_runtime_iat_offset += IAT_INCREMENT;
    }

    /* Load dlls */

    shared_libraries = (WinRuntimeObjectList){
        .allocator = loader_malloc_arena,
    };
    if (!load_dlls(&pe_exe, &shared_libraries)) {
        EXIT("initialize_dynamic_data failed\n");
    }

    runtime_exe = (struct WinRuntimeObject){
        .name = filename,
        .pe_data = pe_exe,
        .memory_regions = memory_regions,
        .function_exports = {},
        .runtime_iat_object_base = runtime_iat_region_base,
        .runtime_iat_section_base = runtime_iat_base,
        .runtime_iat_offset = runtime_iat_offset,
    };

    /* Map Import Address Tables */

    if (tiny_c_munmap(initial_global_runtime_iat_base, IAT_LEN)) {
        EXIT("munmap initial_global_runtime_iat_base failed\n");
    }

    LOADER_LOG("Initializing executable IAT\n");
    if (!initialize_import_address_table(&runtime_exe)) {
        EXIT("initialize_import_address_table failed\n");
    }
    for (size_t i = 0; i < shared_libraries.length; i++) {
        const struct WinRuntimeObject *shared_library =
            &shared_libraries.data[i];
        LOADER_LOG("Initializing '%s' IAT\n", shared_library->name);
        if (!initialize_import_address_table(shared_library)) {
            EXIT("initialize_import_address_table failed\n");
        }
    }

    log_memory_regions();

    /* Bypass Thread Local Storage */

    size_t *tls_buffer = tiny_c_mmap(
        0, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
    );
    if (tls_buffer == MAP_FAILED) {
        EXIT("TLS memory regions failed\n");
    }

    const struct WinSymbol *initialized_symbol =
        find_win_symbol(pe_exe.symbols, pe_exe.symbols_len, "initialized");
    if (initialized_symbol) {
        LOADER_LOG("Setting 'initialized' for TLS\n");
        struct WinSectionHeader *section_header =
            &pe_exe.section_headers[initialized_symbol->section_index];
        size_t section_offset = section_header->virtual_base_address +
            (size_t)initialized_symbol->value;
        size_t symbol_address = image_base + section_offset;
        size_t *temp_init_ptr = (size_t *)symbol_address;
        *temp_init_ptr = 0x01;
    }
    const struct WinSymbol *refptr__imp__acmdln_symbol = find_win_symbol(
        pe_exe.symbols, pe_exe.symbols_len, ".refptr.__imp__acmdln"
    );
    if (refptr__imp__acmdln_symbol) {
        LOADER_LOG("Setting 'refptr__imp__acmdln_symbol' for TLS\n");
        struct WinSectionHeader *section_header =
            &pe_exe.section_headers[refptr__imp__acmdln_symbol->section_index];
        size_t section_offset = section_header->virtual_base_address +
            (size_t)refptr__imp__acmdln_symbol->value;
        size_t symbol_address = image_base + section_offset;
        size_t *refptr__imp__acmdln = (size_t *)symbol_address;
        size_t *refptr__imp__acmdln2 = (size_t *)*refptr__imp__acmdln;
        *refptr__imp__acmdln2 = (size_t)tls_buffer;
    }

    tls_buffer[0x30 / sizeof(uint64_t)] = (size_t)tls_buffer;
    if (tinyc_sys_arch_prctl(ARCH_SET_GS, (size_t)tls_buffer) != 0) {
        EXIT("tinyc_sys_arch_prctl failed\n");
    }

    /* Jump to program */

    size_t *frame_pointer = (size_t *)argv - 1;
    size_t *inferior_frame_pointer = frame_pointer + 1;
    *inferior_frame_pointer = (size_t)(argc - 1);
    size_t *stack_start = inferior_frame_pointer;
    *stack_start = (size_t)win_loader_implicit_end;
    LOADER_LOG("entrypoint: %x\n", pe_exe.entrypoint);
    LOADER_LOG("frame_pointer: %x\n", frame_pointer);
    LOADER_LOG("stack_start: %x\n", stack_start);
    LOADER_LOG("end func: %x\n", *stack_start);
    LOADER_LOG("------------running program------------\n");

    run_asm(
        (size_t)inferior_frame_pointer, (size_t)stack_start, pe_exe.entrypoint
    );
}
