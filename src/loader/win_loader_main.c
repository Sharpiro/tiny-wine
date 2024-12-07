#include "../tiny_c/tiny_c.h"
#include "./pe_tools.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
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

    struct MemoryRegionsInfo memory_regions_info;
    if (!get_memory_regions_info_win(
            pe_data.section_headers,
            pe_data.section_headers_len,
            pe_data.winpe_header->image_optional_header.image_base,
            &memory_regions_info
        )) {
        EXIT("failed getting memory regions\n");
    }

    for (size_t i = 0; i < memory_regions_info.regions_len; i++) {
        struct MemoryRegion *memory_region = &memory_regions_info.regions[i];
        memory_region->permissions = 4 | 2 | 0;
    }

    if (!map_memory_regions(fd, &memory_regions_info)) {
        EXIT("loader map memory regions failed\n");
    }

    get_memory_regions_info_win(
        pe_data.section_headers,
        pe_data.section_headers_len,
        pe_data.winpe_header->image_optional_header.image_base,
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

        int32_t prot_read = (memory_region->permissions & 4) >> 2;
        int32_t prot_write = memory_region->permissions & 2;
        int32_t prot_execute = ((int32_t)memory_region->permissions & 1) << 2;
        int32_t map_protection = prot_read | prot_write | prot_execute;
        size_t memory_region_len = memory_region->end - memory_region->start;
        if (tiny_c_mprotect(region_start, memory_region_len, map_protection) <
            0) {
            EXIT(
                "tiny_c_mprotect failed, %d, %s\n",
                tinyc_errno,
                tinyc_strerror(tinyc_errno)
            );
        }
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
