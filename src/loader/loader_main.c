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
size_t shared_libraries_len;

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

bool find_symbol(
    const char *symbol_name, struct Symbol *symbol, size_t *dynamic_offset
) {
    if (symbol == NULL) {
        BAIL("symbol was null");
    }
    if (dynamic_offset == NULL) {
        BAIL("dynamic_offset was null");
    }

    for (size_t i = 0; i < shared_libraries_len; i++) {
        struct SharedLibrary curr_lib = shared_libraries[i];
        size_t symbols_len = curr_lib.elf_data.dynamic_data->symbols_len;
        for (size_t j = 0; j < symbols_len; j++) {
            struct Symbol curr_symbol =
                curr_lib.elf_data.dynamic_data->symbols[j];
            if (tiny_c_strcmp(curr_symbol.name, symbol_name) == 0) {
                *symbol = curr_symbol;
                *dynamic_offset = curr_lib.dynamic_offset;
                return true;
            }
        }
    }

    return false;
}

void unknown_dynamic_linker_callback(void) {
    tiny_c_fprintf(STDERR, "unknown dynamic linker callback\n");
    tiny_c_exit(-1);
}

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

    struct Relocation *relocation = NULL;
    for (size_t i = 0; i < inferior_elf.dynamic_data->relocations_len; i++) {
        struct Relocation *current_relocation =
            &inferior_elf.dynamic_data->relocations[i];
        if (current_relocation->offset == (size_t)got_entry) {
            relocation = current_relocation;
            break;
        }
    }
    if (relocation == NULL) {
        tiny_c_fprintf(STDERR, "couldn't find relocation\n");
        tiny_c_exit(-1);
    }

    struct Symbol symbol;
    size_t dynamic_offset;
    if (!find_symbol(relocation->symbol.name, &symbol, &dynamic_offset)) {
        tiny_c_fprintf(
            STDERR, "symbol '%s' not found\n", relocation->symbol.name
        );
        tiny_c_exit(-1);
    }

    *got_entry = dynamic_offset + symbol.value;

    LOADER_LOG(
        "dynamically linking %x:%x: '%s'\n",
        got_entry,
        *got_entry,
        relocation->symbol.name
    );
    LOADER_LOG("params: %x, %x, %x, %x, %x, %x\n", r0, r1, r2, r3, r4, r5);

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

bool initialize_dynamic_data(
    struct SharedLibrary **shared_libraries, size_t *shared_libraries_len
) {
    LOADER_LOG("initializing dynamic data\n");
    if (shared_libraries == NULL) {
        BAIL("shared_libraries was null\n");
    }
    if (shared_libraries_len == NULL) {
        BAIL("shared_libraries_len was null\n");
    }

    struct DynamicData *dynamic_data = inferior_elf.dynamic_data;

    /* Map shared libraries */
    *shared_libraries = loader_malloc_arena(
        sizeof(struct SharedLibrary) * dynamic_data->shared_libraries_len
    );
    for (size_t i = 0; i < dynamic_data->shared_libraries_len; i++) {
        char *shared_lib_name = dynamic_data->shared_libraries[i];
        LOADER_LOG("mapping shared library '%s'\n", shared_lib_name);
        int32_t shared_lib_file = tiny_c_open(shared_lib_name, O_RDONLY);
        if (shared_lib_file == -1) {
            BAIL("failed opening shared lib '%s'", shared_lib_name);
        }
        struct ElfData shared_lib_elf;
        if (!get_elf_data(shared_lib_file, &shared_lib_elf)) {
            BAIL("failed getting elf data for shared lib '%s'");
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

        if (!map_memory_regions(
                shared_lib_file, memory_regions, memory_regions_len
            )) {
            BAIL("loader map memory regions failed\n");
        }

        // @todo: adjust library global offset table entries

        struct SharedLibrary shared_library = {
            .name = shared_lib_name,
            .dynamic_offset = dynamic_offset,
            .elf_data = shared_lib_elf,
            .memory_regions = memory_regions,
            .memory_regions_len = memory_regions_len,
        };
        (*shared_libraries)[i] = shared_library;
    }

    /* Initialize dynamic linker callback */
    if (dynamic_data->got_len != 4) {
        BAIL("@todo: got_len\n");
    }
    size_t *loader_entry_one = (void *)dynamic_data->got_entries[1].index;
    *loader_entry_one = (size_t)unknown_dynamic_linker_callback;
    size_t *loader_entry_two = (void *)dynamic_data->got_entries[2].index;
    *loader_entry_two = (size_t)dynamic_linker_callback;

    *shared_libraries_len = dynamic_data->shared_libraries_len;
    return true;
}

int main(int32_t argc, char **argv) {
    int32_t null_file_handle = tiny_c_open("/dev/null", O_RDONLY);
    if (argc > 2 && tiny_c_strcmp(argv[2], "silent") == 0) {
        loader_log_handle = null_file_handle;
    }

    int32_t pid = tiny_c_get_pid();
    LOADER_LOG("pid: %x\n", pid);

    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        return -1;
    }

    char *filename = argv[1];

    LOADER_LOG("argc: %x\n", argc);
    LOADER_LOG("filename: '%s'\n", filename);

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

    // if (elf_data.header.e_type != ET_EXEC) {
    //     BAIL("Program type '%x' not supported\n", elf_data.header.e_type);
    // }

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
