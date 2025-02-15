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

#ifdef AMD64

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

// @note: unclear why some docs consider r10 to be 4th param instead of rcx
void dynamic_callback_linux(void) {
    size_t *rbp;
    __asm__("mov %0, rbp" : "=r"(rbp));
    size_t *p1;
    __asm__("mov %0, rdi" : "=r"(p1));
    size_t *p2;
    __asm__("mov %0, rsi" : "=r"(p2));
    size_t *p3;
    __asm__("mov %0, rdx" : "=r"(p3));
    size_t *p4;
    __asm__("mov %0, rcx" : "=r"(p4));
    size_t *p5;
    __asm__("mov %0, r8" : "=r"(p5));
    size_t *p6;
    __asm__("mov %0, r9" : "=r"(p6));
    size_t p7 = *(rbp + 4);
    size_t p8 = *(rbp + 5);

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
    if (!get_runtime_symbol(
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
        p1,
        p2,
        p3,
        p4,
        p5,
        p6,
        p7,
        p8
    );
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
            "add rsp, 16\n"
            "jmp r15\n" ::"r"(runtime_symbol->value),
            "r"(p1),
            "r"(p2),
            "r"(p3),
            "r"(p4),
            "r"(p5),
            "r"(p6)
            : "r15", "rdi", "rsi", "rdx", "rcx", "r8", "r9");
}

#endif

#ifdef ARM32

static void run_asm(
    size_t frame_start, size_t stack_start, size_t program_entry
) {
    __asm__("mov r0, #0\n"
            "mov r1, #0\n"
            "mov r2, #0\n"
            "mov r3, #0\n"
            "mov r4, #0\n"
            "mov r5, #0\n"
            "mov r6, #0\n"
            "mov r7, #0\n"
            "mov r8, #0\n"
            "mov r9, #0\n"
            "mov r10, #0\n"
            :
            :
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10"
    );

    __asm__("mov fp, %[frame_start]\n"
            "mov sp, %[stack_start]\n"
            "bx %[program_entry]\n"
            :
            : [frame_start] "r"(frame_start),
              [stack_start] "r"(stack_start),
              [program_entry] "r"(program_entry)
            :);
}

// @todo: $fp is now broken and pointing to $sp in arm32?
void dynamic_linker_callback(void) {
    __asm__("mov r10, r0\n");
    size_t r0 = GET_REGISTER("r10");
    size_t r1 = GET_REGISTER("r1");
    size_t r2 = GET_REGISTER("r2");
    size_t r3 = GET_REGISTER("r3");
    size_t r4 = GET_REGISTER("r4");
    size_t r5 = GET_REGISTER("r5");
    size_t *got_entry = (size_t *)GET_REGISTER("r12");

    LOADER_LOG(
        "dynamically linking %x:%x from %x\n",
        got_entry,
        *got_entry,
        dynamic_linker_callback
    );

    const struct RuntimeRelocation *runtime_relocation;
    if (!find_runtime_relocation(
            runtime_func_relocations,
            runtime_func_relocations_len,
            (size_t)got_entry,
            &runtime_relocation
        )) {
        EXIT("relocation %x not found\n", got_entry);
    }
    if (!get_runtime_address(
            runtime_relocation->name,
            runtime_dyn_symbols,
            runtime_dyn_symbols_len,
            got_entry
        )) {
        EXIT("couldn't find runtime symbol '%s'\n", runtime_relocation->name);
    }

    LOADER_LOG(
        "%x: %s(%x, %x, %x, %x, %x, %x)\n",
        *got_entry,
        runtime_relocation->name,
        r0,
        r1,
        r2,
        r3,
        r4,
        r5
    );

    __asm__(
        "mov r10, %0\n"
        "mov r0, %1\n"
        "mov r1, %2\n"
        "mov r2, %3\n"
        "mov r3, %4\n"
        "mov r4, %5\n"
        "mov r5, %6\n"
        "mov sp, fp\n"
        "pop {fp, r12, lr}\n"
        "bx r10\n" ::"r"(*got_entry),
        "r"(r0),
        "r"(r1),
        "r"(r2),
        "r"(r3),
        "r"(r4),
        "r"(r5)
    );
}

#endif

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
        if (!get_runtime_symbols(
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
        dynamic_lib_offset = memory_regions_info.end;
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
        if (!get_runtime_symbol(
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
        if (!get_runtime_symbol(
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

    // @todo: need to sync this with clib
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

    // @todo: old gcc maps this by default for some reason
    const size_t ADDRESS = 0x10000;
    if (tiny_c_munmap(ADDRESS, 0x1000)) {
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

        if (!get_runtime_symbols(
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
    size_t *frame_pointer = (size_t *)argv - 1;
    size_t *inferior_frame_pointer = frame_pointer + 1;
    *inferior_frame_pointer = (size_t)(argc - 1);
    size_t *stack_start = inferior_frame_pointer;
    LOADER_LOG("frame_pointer: %x\n", frame_pointer);
    LOADER_LOG("stack_start: %x\n", stack_start);
    LOADER_LOG("------------running program------------\n");

    run_asm(
        (size_t)inferior_frame_pointer,
        (size_t)stack_start,
        inferior_elf.header.e_entry
    );
}
