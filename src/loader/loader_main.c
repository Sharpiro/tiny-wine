#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

static struct ElfData inferior_elf;
struct SharedLibrary *shared_libraries;
size_t shared_libraries_len = 0;
struct RuntimeRelocation *runtime_func_relocations;
size_t runtime_func_relocations_len = 0;
struct RuntimeRelocation *runtime_var_relocations;
size_t runtime_var_relocations_len = 0;
struct RuntimeSymbol *runtime_dyn_symbols;
size_t runtime_dyn_symbols_len = 0;
struct GotEntry *runtime_got_entries;
size_t runtime_got_entries_len = 0;

#ifdef AMD64

#define ELF_HEADER Elf64_Ehdr

static void run_asm(size_t stack_start, size_t program_entry) {
    __asm__("mov rbx, 0x00\n"
            // set stack pointer
            "mov rsp, %[stack_start]\n"

            //  clear 'PF' flag
            "mov r15, 0xff\n"
            "xor r15, 1\n"

            // clear registers
            "mov rax, 0x00\n"
            "mov rcx, 0x00\n"
            "mov rdx, 0x00\n"
            "mov rsi, 0x00\n"
            "mov rdi, 0x00\n"
            //  "mov rbp, 0x00\n"
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

    // jump to program
    __asm__("jmp %[program_entry]\n"
            :
            : [program_entry] "r"(program_entry)
            : "rax");
}

#endif

#ifdef ARM32

ARM32_START_FUNCTION

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

#endif

// @todo: $fp is now broken and pointing to $sp
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
        tiny_c_fprintf(STDERR, "relocation %x not found\n", got_entry);
        tiny_c_exit(-1);
    }
    if (!get_runtime_address(
            runtime_relocation->name,
            runtime_dyn_symbols,
            runtime_dyn_symbols_len,
            got_entry
        )) {
        tiny_c_fprintf(
            STDERR,
            "couldn't find runtime symbol '%s'\n",
            runtime_relocation->name
        );
        tiny_c_exit(-1);
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

static bool initialize_dynamic_data(
    struct SharedLibrary **shared_libraries, size_t *shared_libraries_len
) {
    LOADER_LOG("initializing dynamic data\n");
    if (shared_libraries == NULL) {
        BAIL("shared_libraries was null\n");
    }
    if (shared_libraries_len == NULL) {
        BAIL("shared_libraries_len was null\n");
    }

    struct DynamicData *inferior_dyn_data = inferior_elf.dynamic_data;

    /* Map shared libraries */
    *shared_libraries_len = inferior_dyn_data->shared_libraries_len;
    runtime_func_relocations_len = inferior_dyn_data->func_relocations_len;
    runtime_var_relocations_len = inferior_dyn_data->var_relocations_len;
    runtime_dyn_symbols_len = inferior_dyn_data->symbols_len;
    runtime_got_entries_len = inferior_dyn_data->got_len;
    *shared_libraries = loader_malloc_arena(
        sizeof(struct SharedLibrary) * inferior_dyn_data->shared_libraries_len
    );
    if (shared_libraries == NULL) {
        BAIL("malloc failed\n");
    }

    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        char *shared_lib_name = inferior_dyn_data->shared_libraries[i];
        LOADER_LOG("mapping shared library '%s'\n", shared_lib_name);
        int32_t shared_lib_file = tiny_c_open(shared_lib_name, O_RDONLY);
        if (shared_lib_file == -1) {
            BAIL("failed opening shared lib '%s'", shared_lib_name);
        }
        struct ElfData shared_lib_elf;
        if (!get_elf_data(shared_lib_file, &shared_lib_elf)) {
            BAIL("failed getting elf data for shared lib '%s'");
        }
        if (shared_lib_elf.dynamic_data == NULL) {
            BAIL("Expected shared library to have dynamic data\n");
        }

        struct MemoryRegion *memory_regions;
        size_t memory_regions_len;
        size_t dynamic_offset = LOADER_SHARED_LIB_START + i * 0x2000;
        if (!get_memory_regions(
                shared_lib_elf.program_headers,
                shared_lib_elf.header.e_phnum,
                &memory_regions,
                &memory_regions_len,
                dynamic_offset
            )) {
            BAIL("failed getting memory regions\n");
        }

        LOADER_LOG("Mapping library memory regions\n");
        if (!map_memory_regions(
                shared_lib_file, memory_regions, memory_regions_len
            )) {
            BAIL("loader map memory regions failed\n");
        }

        runtime_func_relocations_len +=
            shared_lib_elf.dynamic_data->func_relocations_len;
        runtime_var_relocations_len +=
            shared_lib_elf.dynamic_data->var_relocations_len;
        runtime_dyn_symbols_len += shared_lib_elf.dynamic_data->symbols_len;
        runtime_got_entries_len += shared_lib_elf.dynamic_data->got_len;
        struct SharedLibrary shared_library = {
            .name = shared_lib_name,
            .dynamic_offset = dynamic_offset,
            .elf_data = shared_lib_elf,
            .memory_regions = memory_regions,
            .memory_regions_len = memory_regions_len,
        };
        (*shared_libraries)[i] = shared_library;
    }

    /** Get runtime function relocations */
    runtime_func_relocations = loader_malloc_arena(
        sizeof(struct RuntimeRelocation) * runtime_func_relocations_len
    );
    if (runtime_func_relocations == NULL) {
        BAIL("malloc failed\n");
    }

    for (size_t i = 0; i < inferior_dyn_data->func_relocations_len; i++) {
        struct Relocation curr_relocation =
            inferior_dyn_data->func_relocations[i];
        struct RuntimeRelocation runtime_relocation = {
            .offset = curr_relocation.offset,
            .value = curr_relocation.symbol.value,
            .name = curr_relocation.symbol.name,
        };
        runtime_func_relocations[i] = runtime_relocation;
    }

    size_t func_reloc_index = inferior_dyn_data->func_relocations_len;
    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        struct SharedLibrary *curr_lib = &(*shared_libraries)[i];
        struct DynamicData *shared_dyn_data = curr_lib->elf_data.dynamic_data;
        for (size_t i = 0; i < shared_dyn_data->func_relocations_len; i++) {
            struct Relocation *curr_relocation =
                &shared_dyn_data->func_relocations[i];
            size_t value = curr_relocation->symbol.value == 0
                ? 0
                : curr_lib->dynamic_offset + curr_relocation->symbol.value;
            struct RuntimeRelocation runtime_relocation = {
                .offset = curr_lib->dynamic_offset + curr_relocation->offset,
                .value = value,
                .name = curr_relocation->symbol.name,
            };
            runtime_func_relocations[func_reloc_index++] = runtime_relocation;
        }
    }

    /** Get runtime function relocations */
    runtime_var_relocations = loader_malloc_arena(
        sizeof(struct RuntimeRelocation) * runtime_var_relocations_len
    );
    if (runtime_var_relocations == NULL) {
        BAIL("malloc failed\n");
    }

    for (size_t i = 0; i < inferior_dyn_data->var_relocations_len; i++) {
        struct Relocation curr_relocation =
            inferior_dyn_data->var_relocations[i];
        struct RuntimeRelocation runtime_relocation = {
            .offset = curr_relocation.offset,
            .value = curr_relocation.symbol.value,
            .name = curr_relocation.symbol.name,
        };
        runtime_var_relocations[i] = runtime_relocation;
    }

    size_t var_reloc_index = inferior_dyn_data->var_relocations_len;
    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        struct SharedLibrary *curr_lib = &(*shared_libraries)[i];
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
            };
            runtime_var_relocations[var_reloc_index++] = runtime_relocation;
        }
    }

    /** Get runtime symbols */
    runtime_dyn_symbols = loader_malloc_arena(
        sizeof(struct RuntimeSymbol) * runtime_dyn_symbols_len
    );
    if (runtime_dyn_symbols == NULL) {
        BAIL("malloc failed\n");
    }

    for (size_t i = 0; i < inferior_dyn_data->symbols_len; i++) {
        struct Symbol curr_symbol = inferior_dyn_data->symbols[i];
        struct RuntimeSymbol runtime_symbol = {
            .value = curr_symbol.value,
            .name = curr_symbol.name,

        };
        runtime_dyn_symbols[i] = runtime_symbol;
    }

    size_t symbol_index = inferior_dyn_data->symbols_len;
    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        struct SharedLibrary *curr_lib = &(*shared_libraries)[i];
        struct DynamicData *shared_dyn_data = curr_lib->elf_data.dynamic_data;
        for (size_t i = 0; i < shared_dyn_data->symbols_len; i++) {
            struct Symbol *curr_symbol = &shared_dyn_data->symbols[i];
            struct RuntimeSymbol runtime_symbol = {
                .value = curr_lib->dynamic_offset + curr_symbol->value,
                .name = curr_symbol->name,

            };
            runtime_dyn_symbols[symbol_index++] = runtime_symbol;
        }
    }

    /** Get runtime GOT */
    runtime_got_entries =
        tinyc_malloc_arena(sizeof(struct GotEntry) * runtime_got_entries_len);
    if (runtime_got_entries == NULL) {
        BAIL("malloc failed\n");
    }

    for (size_t i = 0; i < inferior_dyn_data->got_len; i++) {
        struct GotEntry *curr_got_entry = &inferior_dyn_data->got_entries[i];
        size_t value = curr_got_entry->is_loader_callback
            ? (size_t)dynamic_linker_callback
            : curr_got_entry->value == 0 ? 0
                                         : curr_got_entry->value;
        struct GotEntry got_entry = {
            .index = curr_got_entry->index,
            .value = value,
            .is_loader_callback = curr_got_entry->is_loader_callback,
        };

        runtime_got_entries[i] = got_entry;
    }

    size_t got_index = inferior_dyn_data->got_len;
    for (size_t i = 0; i < inferior_dyn_data->shared_libraries_len; i++) {
        struct SharedLibrary *curr_lib = &(*shared_libraries)[i];
        struct DynamicData *shared_dyn_data = curr_lib->elf_data.dynamic_data;
        for (size_t i = 0; i < shared_dyn_data->got_len; i++) {
            struct GotEntry *curr_got_entry = &shared_dyn_data->got_entries[i];
            size_t value = curr_got_entry->is_loader_callback
                ? (size_t)dynamic_linker_callback
                : curr_got_entry->value == 0
                ? 0
                : curr_lib->dynamic_offset + curr_got_entry->value;
            struct GotEntry got_entry = {
                .index = curr_lib->dynamic_offset + curr_got_entry->index,
                .value = value,
                .is_loader_callback = curr_got_entry->is_loader_callback,
            };

            runtime_got_entries[got_index++] = got_entry;
        }
    }

    /** Initialize GOT */
    for (size_t i = 0; i < runtime_got_entries_len; i++) {
        struct GotEntry *runtime_got_entry = &runtime_got_entries[i];
        LOADER_LOG(
            "GOT entry %x == %x\n",
            runtime_got_entry->index,
            runtime_got_entry->value
        );
        size_t *got_pointer = (size_t *)runtime_got_entry->index;
        *got_pointer = runtime_got_entry->value;
    }

    for (size_t i = 0; i < runtime_var_relocations_len; i++) {
        struct RuntimeRelocation *runtime_relocation =
            &runtime_var_relocations[i];
        size_t relocation_address;
        if (!get_runtime_address(
                runtime_relocation->name,
                runtime_dyn_symbols,
                runtime_dyn_symbols_len,
                &relocation_address
            )) {
            BAIL(
                "runtime variable relocation '%s' not found\n",
                runtime_relocation->name
            );
        }

        LOADER_LOG(
            "Varaible relocation: %x:%x\n",
            runtime_relocation->offset,
            relocation_address
        );
        size_t *got_pointer = (size_t *)runtime_relocation->offset;
        *got_pointer = relocation_address;
    }

    return true;
}

int main(int32_t argc, char **argv) {
    int32_t null_file_handle = tiny_c_open("/dev/null", O_RDONLY);
    if (argc > 2 && tiny_c_strcmp(argv[2], "silent") == 0) {
        loader_log_handle = null_file_handle;
    }

    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        return -1;
    }

    char *filename = argv[1];

    LOADER_LOG("Starting loader, %s, %x\n", filename, argc);

    int32_t pid = tiny_c_get_pid();
    LOADER_LOG("pid: %x\n", pid);

    // @todo: old gcc maps this by default for some reason
    const size_t ADDRESS = 0x10000;
    if (tiny_c_munmap(ADDRESS, 0x1000)) {
        tiny_c_fprintf(STDERR, "munmap of self failed\n");
        return -1;
    }

    int32_t fd = tiny_c_open(filename, O_RDONLY);
    if (fd < 0) {
        tiny_c_fprintf(
            STDERR,
            "file error, %x, %s\n",
            tinyc_errno,
            tinyc_strerror(tinyc_errno)
        );
        return -1;
    }

    if (!get_elf_data(fd, &inferior_elf)) {
        tiny_c_fprintf(STDERR, "error parsing elf data\n");
        return -1;
    }

    if (inferior_elf.header.e_type != ET_EXEC) {
        BAIL("Program type '%x' not supported\n", inferior_elf.header.e_type);
    }

    LOADER_LOG("program entry: %x\n", inferior_elf.header.e_entry);

    struct MemoryRegion *memory_regions;
    size_t memory_regions_len;
    if (!get_memory_regions(
            inferior_elf.program_headers,
            inferior_elf.header.e_phnum,
            &memory_regions,
            &memory_regions_len,
            0
        )) {
        tiny_c_fprintf(STDERR, "failed getting memory regions\n");
        return -1;
    }

    if (!map_memory_regions(fd, memory_regions, memory_regions_len)) {
        tiny_c_fprintf(STDERR, "loader map memory regions failed\n");
        return -1;
    }

    /* Initialize .bss */
    const struct SectionHeader *bss_section_header = find_section_header(
        inferior_elf.section_headers, inferior_elf.section_headers_len, ".bss"
    );
    if (bss_section_header != NULL) {
        LOADER_LOG("initializing .bss\n");
        void *bss = (void *)bss_section_header->addr;
        memset(bss, 0, bss_section_header->size);
    }

    if (inferior_elf.dynamic_data != NULL) {
        if (!initialize_dynamic_data(
                &shared_libraries, &shared_libraries_len
            )) {
            tiny_c_fprintf(STDERR, "failed initializing dynamic data\n");
            return -1;
        }
    }

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
