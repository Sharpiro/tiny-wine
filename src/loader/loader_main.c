#include "../dlls/macros.h"
#include "../tiny_c/tiny_c.h"
#include "../tiny_c/tinyc_sys.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

struct RuntimeObject *executable_object;
struct RuntimeObject *shared_objects;
size_t shared_libraries_len = 0;
RuntimeSymbolList runtime_symbols;
size_t got_lib_dyn_offset_table[100] = {};

// @todo: loaders shouldn't depend on clib, even statically

// @note: unclear why some docs consider r10 to be 4th param instead of rcx
void dynamic_callback_linux(void) {
    size_t rbx, rcx, rdx, rdi, rsi, r8, r9, r12, r13, r14, r15, *rbp;
    GET_PRESERVED_REGISTERS();
    size_t p7_stack1 = *(rbp + 4);
    size_t p8_stack2 = *(rbp + 5);

    LOADER_LOG("starting dynamic linking at %x\n", dynamic_callback_linux);

    size_t *lib_dyn_offset = (size_t *)(*(rbp + 1));
    size_t relocation_index = *(rbp + 2);
    if (lib_dyn_offset == NULL) {
        EXIT("lib_dyn_offset was null\n");
    }

    LOADER_LOG(
        "relocation params: %x, %x\n", *lib_dyn_offset, relocation_index
    );
    struct RuntimeRelocation *runtime_relocation;
    if (*lib_dyn_offset == 0) {
        runtime_relocation =
            &executable_object->runtime_func_relocations[relocation_index];
    } else {
        for (size_t i = 0; i < shared_libraries_len; i++) {
            struct RuntimeObject *current_lib = &shared_objects[i];
            if (current_lib->dynamic_offset != *lib_dyn_offset) {
                continue;
            }
            if (relocation_index >= current_lib->runtime_func_relocations_len) {
                EXIT(
                    "relocation index %d is not less than %d\n",
                    relocation_index,
                    current_lib->runtime_func_relocations_len
                );
            }

            runtime_relocation =
                &current_lib->runtime_func_relocations[relocation_index];
        }
    }

    size_t *got_entry = (size_t *)runtime_relocation->offset;
    LOADER_LOG("got_entry: %x: %x\n", got_entry, *got_entry);

    const struct RuntimeSymbol *runtime_symbol;
    if (!find_runtime_symbol(
            runtime_relocation->name,
            runtime_symbols.data,
            runtime_symbols.length,
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
        rdi,
        rsi,
        rdx,
        rcx,
        r8,
        r9,
        p7_stack1,
        p8_stack2
    );
    LOADER_LOG("completed dynamic linking\n");

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

        "mov rbx, %[rbx]\n"
        "mov rsp, rbp\n"
        "pop rbp\n"
        "add rsp, 16\n"
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
        [r15] "m"(r15)
    );
}

static bool initialize_dynamic_data(
    struct DynamicData *inferior_dyn_data,
    struct RuntimeObject **shared_libraries,
    size_t *shared_libraries_len
) {
    LOADER_LOG("initializing dynamic data\n");
    if (shared_libraries == NULL) {
        BAIL("shared_libraries was null\n");
    }
    if (shared_libraries_len == NULL) {
        BAIL("shared_libraries_len was null\n");
    }

    /* Map shared libraries */

    *shared_libraries_len = inferior_dyn_data->shared_libraries_len;
    size_t runtime_var_relocations_len = inferior_dyn_data->var_relocations_len;
    *shared_libraries = loader_malloc_arena(
        sizeof(struct RuntimeObject) * inferior_dyn_data->shared_libraries_len
    );
    if (shared_libraries == NULL) {
        BAIL("loader_malloc_arena failed\n");
    }

    size_t dynamic_lib_offset = LOADER_SHARED_LIB_START;
    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        char *shared_lib_name = inferior_dyn_data->shared_libraries[i];
        LOADER_LOG("mapping shared library '%s'\n", shared_lib_name);
        int32_t shared_lib_file = tiny_c_open(shared_lib_name, O_RDONLY);
        if (shared_lib_file == -1) {
            BAIL("failed opening shared lib '%s'\n", shared_lib_name);
        }

        struct ElfData shared_lib_elf;
        if (!get_elf_data(shared_lib_file, &shared_lib_elf)) {
            BAIL(
                "failed getting elf data for shared lib '%s'\n", shared_lib_name
            );
        }
        if (shared_lib_elf.dynamic_data == NULL) {
            BAIL("Expected shared library to have dynamic data\n");
        }

        struct MemoryRegionsInfo memory_regions_info;
        if (!get_memory_regions_info_x86(
                shared_lib_elf.program_headers,
                shared_lib_elf.header.e_phnum,
                dynamic_lib_offset,
                &memory_regions_info
            )) {
            BAIL("failed getting memory regions\n");
        }

        LOADER_LOG("Mapping library memory regions\n");
        if (!map_memory_regions(
                shared_lib_file,
                memory_regions_info.regions,
                memory_regions_info.regions_len
            )) {
            BAIL("loader lib map memory regions failed\n");
        }

        tiny_c_close(shared_lib_file);
        if (!log_memory_regions()) {
            BAIL("print_memory_regions failed\n");
        }

        runtime_var_relocations_len +=
            shared_lib_elf.dynamic_data->var_relocations_len;

        uint8_t *bss = 0;
        size_t bss_len = 0;
        const struct SectionHeader *bss_section_header = find_section_header(
            shared_lib_elf.section_headers,
            shared_lib_elf.section_headers_len,
            ".bss"
        );
        if (bss_section_header != NULL) {
            bss = (uint8_t *)(dynamic_lib_offset + bss_section_header->addr);
            bss_len = bss_section_header->size;
        }

        /** Get runtime function relocations */

        struct RuntimeRelocation *runtime_func_relocations;
        size_t runtime_func_relocations_len;
        if (!get_function_relocations(
                shared_lib_elf.dynamic_data,
                dynamic_lib_offset,
                &runtime_func_relocations,
                &runtime_func_relocations_len
            )) {
            BAIL("get_function_relocations failed\n");
        }

        /** Get runtime library symbols */

        RuntimeSymbolList runtime_lib_symbols = (RuntimeSymbolList){
            .allocator = loader_malloc_arena,
        };
        if (!find_win_symbols(
                shared_lib_elf.dynamic_data,
                dynamic_lib_offset,
                &runtime_lib_symbols
            )) {
            BAIL("failed getting symbols\n");
        }

        struct RuntimeObject shared_library = {
            .name = shared_lib_name,
            .dynamic_offset = dynamic_lib_offset,
            .elf_data = shared_lib_elf,
            .memory_regions_info = memory_regions_info,
            .runtime_func_relocations = runtime_func_relocations,
            .runtime_func_relocations_len = runtime_func_relocations_len,
            .bss = bss,
            .bss_len = bss_len,
            .runtime_symbols = runtime_lib_symbols,
        };
        (*shared_libraries)[i] = shared_library;
        dynamic_lib_offset = memory_regions_info.regions_len == 0
            ? 0
            : memory_regions_info.regions[memory_regions_info.regions_len - 1]
                  .end;
    }

    /** Get runtime symbols */

    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        struct RuntimeObject *curr_lib = &(*shared_libraries)[i];
        for (size_t i = 0; i < curr_lib->runtime_symbols.length; i++) {
            struct RuntimeSymbol *runtime_symbol =
                &curr_lib->runtime_symbols.data[i];
            RuntimeSymbolList_add(&runtime_symbols, *runtime_symbol);
        }
    }

    /** Get runtime variable relocations */

    struct RuntimeRelocation *runtime_var_relocations = loader_malloc_arena(
        sizeof(struct RuntimeRelocation) * runtime_var_relocations_len
    );
    if (runtime_var_relocations == NULL) {
        BAIL("loader_malloc_arena failed\n");
    }

    for (size_t i = 0; i < inferior_dyn_data->var_relocations_len; i++) {
        struct Relocation *curr_relocation =
            &inferior_dyn_data->var_relocations[i];
        struct RuntimeRelocation runtime_relocation = {
            .offset = curr_relocation->offset,
            .value = curr_relocation->symbol.value,
            .name = curr_relocation->symbol.name,
            .type = curr_relocation->type,
        };
        LOADER_LOG(
            "variable relocation %d: %s %x:%x, type %d\n",
            i + 1,
            runtime_relocation.name,
            runtime_relocation.offset,
            runtime_relocation.value,
            runtime_relocation.type
        );
        runtime_var_relocations[i] = runtime_relocation;
    }

    size_t var_reloc_index = inferior_dyn_data->var_relocations_len;
    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        struct RuntimeObject *curr_lib = &(*shared_libraries)[i];
        struct DynamicData *shared_dyn_data = curr_lib->elf_data.dynamic_data;
        for (size_t i = 0; i < shared_dyn_data->var_relocations_len; i++) {
            struct Relocation *curr_relocation =
                &shared_dyn_data->var_relocations[i];
            size_t value = curr_relocation->symbol.value == 0
                ? 0
                : curr_lib->dynamic_offset + curr_relocation->symbol.value;
            struct RuntimeRelocation runtime_relocation = {
                .offset = curr_lib->dynamic_offset + curr_relocation->offset,
                .value = value,
                .name = curr_relocation->symbol.name,
                .type = curr_relocation->type,
            };
            LOADER_LOG(
                "Varaible relocation: %d: %s %x:%x\n",
                i + 1,
                runtime_relocation.name,
                runtime_relocation.offset,
                runtime_relocation.value
            );
            runtime_var_relocations[var_reloc_index++] = runtime_relocation;
        }
    }
    for (size_t i = 0; i < runtime_var_relocations_len; i++) {
        struct RuntimeRelocation *run_var_reloc = &runtime_var_relocations[i];
        const struct RuntimeSymbol *runtime_symbol;
        if (!find_runtime_symbol(
                run_var_reloc->name,
                runtime_symbols.data,
                runtime_symbols.length,
                0,
                &runtime_symbol
            )) {
            BAIL(
                "runtime variable relocation '%s' not found\n",
                run_var_reloc->name
            );
        }

        run_var_reloc->value = runtime_symbol->value;
    }

    /** Get runtime GOT */

    RuntimeGotEntryList runtime_got_entries = {
        .allocator = loader_malloc_arena,
    };
    if (!get_runtime_got(
            inferior_dyn_data,
            0,
            (size_t)dynamic_callback_linux,
            got_lib_dyn_offset_table,
            &runtime_got_entries
        )) {
        BAIL("get_runtime_got failed\n");
    }

    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        struct RuntimeObject *curr_lib = &(*shared_libraries)[i];
        struct DynamicData *shared_dyn_data = curr_lib->elf_data.dynamic_data;
        size_t *lib_dyn_offset = got_lib_dyn_offset_table + i + 1;
        if (!get_runtime_got(
                shared_dyn_data,
                curr_lib->dynamic_offset,
                (size_t)dynamic_callback_linux,
                lib_dyn_offset,
                &runtime_got_entries
            )) {
            BAIL("failed getting symbols\n");
        }
    }

    /** Update GOT with variable relocations */

    for (size_t i = 0; i < runtime_var_relocations_len; i++) {
        struct RuntimeRelocation *var_relocation = &runtime_var_relocations[i];
        if (var_relocation->type != R_X86_64_GLOB_DAT) {
            continue;
        }

        struct RuntimeGotEntry *runtime_got_entry;
        if (!find_got_entry(
                runtime_got_entries.data,
                runtime_got_entries.length,
                var_relocation->offset,
                &runtime_got_entry
            )) {
            BAIL("Variable got entry %x not found\n", var_relocation->offset);
        }

        runtime_got_entry->value = var_relocation->value;
    }

    /** Init shared lib .bss */

    for (size_t i = 0; i < *shared_libraries_len; i++) {
        struct RuntimeObject *shared_lib = &(*shared_libraries)[i];
        if (shared_lib->bss != NULL) {
            LOADER_LOG("initializing shared lib .bss\n");
            memset(shared_lib->bss, 0, shared_lib->bss_len);
        }
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

    /** Init variable relocation 'copy' types */

    for (size_t i = 0; i < runtime_var_relocations_len; i++) {
        struct RuntimeRelocation *var_relocation = &runtime_var_relocations[i];
        if (var_relocation->type != R_X86_64_COPY) {
            continue;
        }

        const struct RuntimeSymbol *runtime_symbol;
        if (!find_runtime_symbol(
                var_relocation->name,
                runtime_symbols.data,
                runtime_symbols.length,
                var_relocation->offset,
                &runtime_symbol
            )) {
            BAIL("runtime symbol '%s' not found\n", var_relocation->name);
        }
        size_t *init_value = (size_t *)runtime_symbol->value;
        size_t init_value_sized;
        memcpy(&init_value_sized, init_value, runtime_symbol->size);

        size_t *variable = (size_t *)var_relocation->offset;
        *variable = init_value_sized;
    }

    return true;
}

int main(int32_t argc, char **argv) {
    if (argc < 2) {
        EXIT("Filename required\n", argc);
    }

    char *filename = argv[1];
    LOADER_LOG("Starting loader, %s, %d\n", filename, argc);

    /* Init heap */

    size_t brk_start = tinyc_sys_brk(0);
    LOADER_LOG("BRK:, %x\n", brk_start);
    size_t brk_end = tinyc_sys_brk(brk_start + 0x1000);
    LOADER_LOG("BRK:, %x\n", brk_end);
    if (brk_end <= brk_start) {
        EXIT("program BRK setup failed");
    }

    log_memory_regions();

    int32_t pid = tiny_c_get_pid();
    LOADER_LOG("pid: %d\n", pid);

    /* Unmap default locations */

    if (tiny_c_munmap(0x10000, 0x1000)) {
        EXIT("munmap of self failed\n");
    }
    if (tiny_c_munmap(0x400000, 0x1000)) {
        EXIT("munmap of self failed\n");
    }

    int32_t fd = tiny_c_open(filename, O_RDONLY);
    if (fd < 0) {
        EXIT("file error, %d, %s\n", tinyc_errno, tinyc_strerror(tinyc_errno));
    }

    struct ElfData inferior_elf;
    if (!get_elf_data(fd, &inferior_elf)) {
        EXIT("error parsing elf data\n");
    }

    if (inferior_elf.header.e_type != ET_EXEC) {
        EXIT("Program type '%d' not supported\n", inferior_elf.header.e_type);
    }

    LOADER_LOG("program entry: %x\n", inferior_elf.header.e_entry);

    struct MemoryRegionsInfo memory_regions_info;
    if (!get_memory_regions_info_x86(
            inferior_elf.program_headers,
            inferior_elf.header.e_phnum,
            0,
            &memory_regions_info
        )) {
        EXIT("failed getting memory regions\n");
    }

    if (!map_memory_regions(
            fd, memory_regions_info.regions, memory_regions_info.regions_len
        )) {
        EXIT("loader map memory regions failed\n");
    }

    log_memory_regions();

    const struct SectionHeader *bss_section_header = find_section_header(
        inferior_elf.section_headers, inferior_elf.section_headers_len, ".bss"
    );
    uint8_t *bss = NULL;
    size_t bss_len = 0;
    if (bss_section_header != NULL) {
        LOADER_LOG("initializing .bss\n");
        bss = (uint8_t *)bss_section_header->addr;
        bss_len = bss_section_header->size;
        memset(bss, 0, bss_len);
    }

    struct RuntimeRelocation *runtime_func_relocations = NULL;
    size_t runtime_func_relocations_len = 0;
    RuntimeSymbolList exe_runtime_symbols = (RuntimeSymbolList){
        .allocator = loader_malloc_arena,
    };
    runtime_symbols = (RuntimeSymbolList){
        .allocator = loader_malloc_arena,
    };
    if (inferior_elf.dynamic_data != NULL) {
        if (!get_function_relocations(
                inferior_elf.dynamic_data,
                0,
                &runtime_func_relocations,
                &runtime_func_relocations_len
            )) {
            EXIT("get_function_relocations failed\n");
        }

        if (!find_win_symbols(
                inferior_elf.dynamic_data, 0, &exe_runtime_symbols
            )) {
            EXIT("failed getting symbols\n");
        }
        RuntimeSymbolList_clone(&runtime_symbols, &exe_runtime_symbols);

        if (!initialize_dynamic_data(
                inferior_elf.dynamic_data,
                &shared_objects,
                &shared_libraries_len
            )) {
            EXIT("failed initializing dynamic data\n");
        }
    }

    executable_object = loader_malloc_arena(sizeof(struct RuntimeObject));
    *executable_object = (struct RuntimeObject){
        .name = filename,
        .dynamic_offset = 0,
        .elf_data = inferior_elf,
        .memory_regions_info = memory_regions_info,
        .runtime_func_relocations = runtime_func_relocations,
        .runtime_func_relocations_len = runtime_func_relocations_len,
        .bss = bss,
        .bss_len = bss_len,
        .runtime_symbols = exe_runtime_symbols,
    };

    /* Jump to program */

    size_t *inferior_stack = (size_t *)argv;
    *inferior_stack = (size_t)(argc - 1);

    LOADER_LOG("inferior_entry: %x\n", inferior_elf.header.e_entry);
    LOADER_LOG("inferior_stack: %x\n", inferior_stack);
    LOADER_LOG("------------running program------------\n");

    __asm__(

        ".loader_jump_to_entry:\n"
        "mov rsp, %[inferior_stack]\n"
        "jmp %[inferior_entry]\n"
        :
        :

        [inferior_stack] "m"(inferior_stack),
        [inferior_entry] "m"(inferior_elf.header.e_entry)
    );
}
