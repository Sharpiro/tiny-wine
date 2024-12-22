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

const size_t IAT_BASE_START = 0x7d7e0000;
const size_t DYNAMIC_CALLBACK_TRAMPOLINE_SIZE = 0x0e;

struct PeData pe_data;
struct RuntimeObject *lib_ntdll_object;

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

__attribute__((naked)) void win_loader_end(void) {
    __asm__("mov rdi, rax\n"
            "mov rax, 0x3c\n"
            "syscall\n");
}

void dynamic_callback(void) {
    size_t *rbp;
    __asm__("mov %0, rbp" : "=r"(rbp));
    size_t source_address = 0;
    __asm__("mov %0, r15\n" : "=r"(source_address));
    size_t p1;
    __asm__("mov %0, rcx" : "=r"(p1));
    size_t p2;
    __asm__("mov %0, rdx" : "=r"(p2));
    size_t p3;
    __asm__("mov %0, r8" : "=r"(p3));
    size_t p4;
    __asm__("mov %0, r9" : "=r"(p4));
    size_t p5 = rbp[6];
    size_t p6 = rbp[7];

    size_t func_iat_value = source_address - IAT_BASE_START;
    size_t func_iat_key = 0;

    for (size_t i = 0; i < pe_data.import_address_table_len; i++) {
        struct KeyValue *iat_entry = &pe_data.import_address_table[i];
        if (iat_entry->value == func_iat_value) {
            func_iat_key = iat_entry->key;
        }
    }
    if (func_iat_key == 0) {
        EXIT("dynamic entry offset '%x' not found\n", func_iat_value);
    }

    LOADER_LOG(
        "Dynamic linker callback hit, %x:%x\n", func_iat_key, func_iat_value
    );

    struct ImportEntry *import_entry = NULL;
    for (size_t i = 0; i < pe_data.import_dir_entries_len; i++) {
        struct ImportDirectoryEntry *dir_entry = &pe_data.import_dir_entries[i];
        for (size_t i = 0; i < dir_entry->import_entries_len; i++) {
            struct ImportEntry *current_entry = &dir_entry->import_entries[i];
            if (current_entry->address == func_iat_value) {
                import_entry = current_entry;
            }
        }
    }
    if (import_entry == NULL) {
        EXIT("dynamic entry offset '%x' not found\n", func_iat_value);
    };

    LOADER_LOG(
        "%s(%x, %x, %x, %x, %x, %x)\n",
        import_entry->name,
        p1,
        p2,
        p3,
        p4,
        p5,
        p6
    );

    // @todo: find function
    size_t dynamic_func_address = (size_t)0x00;
    EXIT("unsupported arbitrary function lookup\n", func_iat_value);

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
            "jmp r15\n" ::"r"(dynamic_func_address),
            "r"(p1),
            "r"(p2),
            "r"(p3),
            "r"(p4),
            "r"(p5),
            "r"(p6)
            : "r15", "rdi", "rsi", "rdx", "rcx", "r8", "r9");
}

__attribute__((naked)) void dynamic_callback_trampoline(void) {
    __asm__("lea r15, [rip - 0x0c]\n"
            "jmp %0\n" ::"r"(dynamic_callback));
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
    if (!map_memory_regions(ntdll_file, &memory_regions_info)) {
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

    *lib_ntdll_object = (struct RuntimeObject){
        .name = LIB_NTDLL_SO_NAME,
        // @todo: need runtime tracking for other shared libs
        .dynamic_offset = LOADER_SHARED_LIB_START,
        .elf_data = ntdll_elf,
        .memory_regions_info = memory_regions_info,
        .runtime_func_relocations = runtime_func_relocations,
        .runtime_func_relocations_len = runtime_func_relocations_len,
        .bss = bss,
        .bss_len = bss_len,
    };

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

    if (!get_pe_data(fd, &pe_data)) {
        tiny_c_fprintf(STDERR, "error parsing pe data\n");
        return -1;
    }

    size_t image_base = pe_data.winpe_header->image_optional_header.image_base;

    struct MemoryRegionsInfo memory_regions_info;
    if (!get_memory_regions_info_win(
            pe_data.section_headers,
            pe_data.section_headers_len,
            image_base,
            &memory_regions_info
        )) {
        EXIT("failed getting memory regions\n");
    }

    for (size_t i = 0; i < memory_regions_info.regions_len; i++) {
        struct MemoryRegion *memory_region = &memory_regions_info.regions[i];
        memory_region->permissions = 4 | 2 | 1;
    }

    if (!map_memory_regions(fd, &memory_regions_info)) {
        EXIT("loader map memory regions failed\n");
    }

    /* Protect memory regions */

    get_memory_regions_info_win(
        pe_data.section_headers,
        pe_data.section_headers_len,
        image_base,
        &memory_regions_info
    );
    for (size_t i = 0; i < memory_regions_info.regions_len; i++) {
        struct MemoryRegion *memory_region = &memory_regions_info.regions[i];
        uint8_t *region_start = (uint8_t *)memory_region->start;
        tinyc_lseek(fd, (off_t)memory_region->file_offset, SEEK_SET);
        if ((int32_t)tiny_c_read(fd, region_start, memory_region->file_size) <
            0) {
            EXIT("read failed\n");
        }

        // @todo: needs to happen later
        // int32_t prot_read = (memory_region->permissions & 4) >> 2;
        // int32_t prot_write = memory_region->permissions & 2;
        // int32_t prot_execute = ((int32_t)memory_region->permissions & 1) <<
        // 2; int32_t map_protection = prot_read | prot_write | prot_execute;
        // size_t memory_region_len = memory_region->end - memory_region->start;
        // if (tiny_c_mprotect(region_start, memory_region_len, map_protection)
        // <
        //     0) {
        //     EXIT(
        //         "tiny_c_mprotect failed, %d, %s\n",
        //         tinyc_errno,
        //         tinyc_strerror(tinyc_errno)
        //     );
        // }
    }

    /* Map Import Address Table memory */
    // @todo: is this redundant?

    const struct WinSectionHeader *idata_header = find_win_section_header(
        pe_data.section_headers, pe_data.section_headers_len, ".idata"
    );
    size_t iat_mem_start = idata_header->base_address;
    struct MemoryRegion iat_regions = {
        .start = IAT_BASE_START + iat_mem_start,
        .end = IAT_BASE_START + iat_mem_start + 0x1000,
        .is_direct_file_map = false,
        .file_offset = 0,
        .file_size = 0,
        .permissions = 4 | 2 | 1,
    };
    struct MemoryRegionsInfo iat_mem_info = {
        .start = 0,
        .end = 0,
        .regions = &iat_regions,
        .regions_len = 1,
    };
    if (!map_memory_regions(fd, &iat_mem_info)) {
        EXIT("loader map memory regions failed\n");
    }

    if (DYNAMIC_CALLBACK_TRAMPOLINE_SIZE > 0x10) {
        EXIT("unsupported trampoline size, must be less than 16 bytes\n");
    }

    size_t *iat_offset =
        (size_t *)(image_base + pe_data.import_address_table_offset);
    for (size_t i = 0; i < pe_data.import_address_table_len; i++) {
        size_t *iat_entry = iat_offset + i;
        size_t iat_entry_init = *iat_entry;
        size_t *entry_trampoline = (size_t *)(IAT_BASE_START + *iat_entry);
        *iat_entry = (size_t)entry_trampoline;
        memcpy(
            entry_trampoline,
            (void *)dynamic_callback_trampoline,
            DYNAMIC_CALLBACK_TRAMPOLINE_SIZE
        );
        LOADER_LOG(
            "IAT: %x, %x, %x\n", iat_entry, iat_entry_init, entry_trampoline
        );
    }

    /* Load libntdll.so */

    lib_ntdll_object = loader_malloc_arena(sizeof(struct RuntimeObject));
    if (!initialize_lib_ntdll(lib_ntdll_object)) {
        EXIT("initialize_lib_ntdll failed\n");
    }

    print_memory_regions();

    /* Jump to program */

    size_t *frame_pointer = (size_t *)argv - 1;
    size_t *inferior_frame_pointer = frame_pointer + 1;
    *inferior_frame_pointer = (size_t)(argc - 1);
    size_t *stack_start = inferior_frame_pointer;
    *stack_start = (size_t)win_loader_end;
    LOADER_LOG("entrypoint: %x\n", pe_data.entrypoint);
    LOADER_LOG("frame_pointer: %x\n", frame_pointer);
    LOADER_LOG("stack_start: %x\n", stack_start);
    LOADER_LOG("end func: %x\n", *stack_start);
    LOADER_LOG("------------running program------------\n");

    run_asm(
        (size_t)inferior_frame_pointer, (size_t)stack_start, pe_data.entrypoint
    );
}
