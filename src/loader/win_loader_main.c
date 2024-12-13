#include "../tiny_c/tiny_c.h"
#include "./pe_tools.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

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

void dynamic_linker_callback(void) {
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

    LOADER_LOG("Dynamic linker callback hit, %x\n", source_address);
    LOADER_LOG("func(%x, %x, %x, %x, %x, %x)\n", p1, p2, p3, p4, p5, p6);

    __asm__("mov rax, 0\n");
}

__attribute__((naked)) void dynamic_trampoline(void) {
    __asm__("lea r15, [rip - 0x0c]\n"
            "jmp %0\n" ::"r"(dynamic_linker_callback));
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

    struct PeData pe_data;
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

    const size_t IAT_BASE_START = 0x7d7e0000;
    const size_t IAT_MEM_START = 0x4000;
    struct MemoryRegion iat_regions = {
        .start = IAT_BASE_START + IAT_MEM_START,
        .end = IAT_BASE_START + IAT_MEM_START + 0x1000,
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

    print_memory_regions();

    size_t *iat_offset =
        (size_t *)(image_base + pe_data.import_address_table_offset);
    for (size_t i = 0; i < pe_data.import_address_table_length; i++) {
        size_t *iat_entry = iat_offset + i;
        size_t iat_entry_init = *iat_entry;
        size_t *entry_trampoline = (size_t *)(IAT_BASE_START + *iat_entry);
        *iat_entry = (size_t)entry_trampoline;
        memcpy(entry_trampoline, (void *)dynamic_trampoline, 0x0e);
        LOADER_LOG(
            "IAT: %x, %x, %x\n", iat_entry, iat_entry_init, entry_trampoline
        );
    }

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
