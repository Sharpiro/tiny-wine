#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

static struct ElfData elf_data;
int32_t log_handle = STDERR;

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

void unknown_dynamic_linker_callback(void) {
    tiny_c_printf("unknown dynamic linker callback\n");
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
    for (size_t i = 0; i < elf_data.dynamic_data->relocations_len; i++) {
        struct Relocation *current_relocation =
            &elf_data.dynamic_data->relocations[i];
        if (current_relocation->offset == (size_t)got_entry) {
            relocation = current_relocation;
            break;
        }
    }
    if (relocation == NULL) {
        tiny_c_printf("couldn't find relocation\n");
        tiny_c_exit(-1);
    }

    tiny_c_printf(
        "dynamically linking %x: '%s'\n", got_entry, relocation->symbol.name
    );
    tiny_c_printf("params: %x, %x, %x, %x, %x, %x\n", r0, r1, r2, r3, r4, r5);

    // @todo: set got_entry to function address
    *got_entry = (size_t)tiny_c_printf;

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

int initialize_dynamic_data(void) {
    tiny_c_fprintf(log_handle, "initializing dynamic data\n");
    struct DynamicData *dynamic_data = elf_data.dynamic_data;

    /* Map shared libraries */
    for (size_t i = 0; i < dynamic_data->shared_libraries_len; i++) {
        char *shared_library = dynamic_data->shared_libraries[i];
        tiny_c_fprintf(
            log_handle, "mapping shared library '%s'\n", shared_library
        );
    }

    /* Initialize dynamic linker callback */
    if (dynamic_data->got_len != 4) {
        tiny_c_fprintf(STDERR, "@todo: got_len\n");
        return -1;
    }
    size_t *loader_entry_one = (void *)dynamic_data->got_entries[1].index;
    *loader_entry_one = (size_t)unknown_dynamic_linker_callback;
    size_t *loader_entry_two = (void *)dynamic_data->got_entries[2].index;
    *loader_entry_two = (size_t)dynamic_linker_callback;

    return 0;
}

int main(int32_t argc, char **argv) {
    int32_t null_file_handle = tiny_c_open("/dev/null", O_RDONLY);
    if (argc > 2 && tiny_c_strcmp(argv[2], "silent") == 0) {
        log_handle = null_file_handle;
    }

    int32_t pid = tiny_c_get_pid();
    tiny_c_fprintf(log_handle, "pid: %x\n", pid);

    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        return -1;
    }

    char *filename = argv[1];

    tiny_c_fprintf(log_handle, "argc: %x\n", argc);
    tiny_c_fprintf(log_handle, "filename: '%s'\n", filename);

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

    if (!get_elf_data(fd, &elf_data)) {
        tiny_c_fprintf(STDERR, "error parsing elf data\n");
        return -1;
    }

    // if (elf_data.header.e_type != ET_EXEC) {
    //     BAIL("Program type '%x' not supported\n", elf_data.header.e_type);
    // }

    tiny_c_fprintf(log_handle, "program entry: %x\n", elf_data.header.e_entry);

    struct MemoryRegion *memory_regions;
    size_t memory_regions_len;
    if (!get_memory_regions(
            elf_data.program_headers,
            elf_data.header.e_phnum,
            &memory_regions,
            &memory_regions_len
        )) {
        tiny_c_fprintf(STDERR, "failed getting memory regions\n");
        return -1;
    }

    tiny_c_fprintf(log_handle, "memory regions: %x\n", memory_regions_len);
    for (size_t i = 0; i < memory_regions_len; i++) {
        struct MemoryRegion *memory_region = &memory_regions[i];
        size_t memory_region_len = memory_region->end - memory_region->start;
        size_t prot_read = (memory_region->permissions & 4) >> 2;
        size_t prot_write = memory_region->permissions & 2;
        size_t prot_execute = (memory_region->permissions & 1) << 2;
        size_t map_protection = prot_read | prot_write | prot_execute;
        uint8_t *addr = tiny_c_mmap(
            memory_region->start,
            memory_region_len,
            map_protection,
            MAP_PRIVATE | MAP_FIXED,
            fd,
            memory_region->file_offset
        );
        tiny_c_fprintf(
            log_handle, "map address: %x, %x\n", memory_region->start, addr
        );
        if ((size_t)addr != memory_region->start) {
            tiny_c_fprintf(
                STDERR,
                "map failed, %x, %s\n",
                tinyc_errno,
                tinyc_strerror(tinyc_errno)
            );
            return -1;
        }
    }

    /* Initialize .bss */
    const struct SectionHeader *bss_section_header = find_section_header(
        elf_data.section_headers, elf_data.section_headers_len, ".bss"
    );
    if (bss_section_header != NULL) {
        tiny_c_fprintf(log_handle, "initializing .bss\n");
        void *bss = (void *)bss_section_header->addr;
        memset(bss, 0, bss_section_header->size);
    }

    if (elf_data.dynamic_data != NULL) {
        initialize_dynamic_data();
    }

    /* Jump to program */
    size_t *frame_pointer = (size_t *)argv - 1;
    size_t *inferior_frame_pointer = frame_pointer + 1;
    *inferior_frame_pointer = (size_t)(argc - 1);
    size_t *stack_start = inferior_frame_pointer;
    tiny_c_fprintf(log_handle, "frame_pointer: %x\n", frame_pointer);
    tiny_c_fprintf(log_handle, "stack_start: %x\n", stack_start);
    tiny_c_fprintf(log_handle, "------------running program------------\n");

    run_asm(
        (size_t)inferior_frame_pointer,
        (size_t)stack_start,
        elf_data.header.e_entry
    );
}
