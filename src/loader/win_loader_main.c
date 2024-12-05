#include "../tiny_c/tiny_c.h"
#include "./pe_tools.h"
#include "elf_tools.h"
#include "loader_lib.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

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
    LOADER_LOG("Starting loader, %s, %d\n", filename, argc);

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

    const size_t TEXT_START = 0x140001000;
    const size_t RDATA_START = 0x140002000;

    struct MemoryRegion memory_regions[] = {
        {
            .start = TEXT_START,
            .end = 0x140002000,
            .is_file_map = false,
            .file_offset = 0,
            .permissions = 4 | 2,
        },
        {
            .start = RDATA_START,
            .end = 0x140003000,
            .is_file_map = false,
            .file_offset = 0,
            .permissions = 4 | 2,
        }
    };
    struct MemoryRegionsInfo memory_regions_info = {
        .start = 0,
        .end = 0,
        .regions = memory_regions,
        .regions_len = sizeof(memory_regions) / sizeof(struct MemoryRegion),
    };
    if (!map_memory_regions(fd, &memory_regions_info)) {
        tiny_c_fprintf(STDERR, "loader map memory regions failed\n");
        return -1;
    }

    uint8_t *text_region_start = (uint8_t *)TEXT_START;
    tinyc_lseek(fd, 0x400, SEEK_SET);
    tiny_c_read(fd, text_region_start, 250);

    uint8_t *rdata_region_start = (uint8_t *)RDATA_START;
    tinyc_lseek(fd, 0x600, SEEK_SET);
    tiny_c_read(fd, rdata_region_start, 8);

    int32_t map_permission = 4 | 1;
    int32_t prot_read = (map_permission & 4) >> 2;
    int32_t prot_write = map_permission & 2;
    int32_t prot_execute = (map_permission & 1) << 2;
    int32_t map_protection = prot_read | prot_write | prot_execute;
    if (tiny_c_mprotect(text_region_start, 0x1'000, map_protection) < 0) {
        tiny_c_fprintf(
            STDERR,
            "tiny_c_mprotect failed, %d, %s\n",
            tinyc_errno,
            tinyc_strerror(tinyc_errno)
        );
        return -1;
    }

    map_permission = 4;
    prot_read = (map_permission & 4) >> 2;
    prot_write = map_permission & 2;
    prot_execute = (map_permission & 1) << 2;
    map_protection = prot_read | prot_write | prot_execute;
    if (tiny_c_mprotect(rdata_region_start, 0x1'000, map_protection) < 0) {
        tiny_c_fprintf(
            STDERR,
            "tiny_c_mprotect failed, %d, %s\n",
            tinyc_errno,
            tinyc_strerror(tinyc_errno)
        );
        return -1;
    }

    print_memory_regions();

    /* Jump to program */
    size_t *frame_pointer = (size_t *)argv - 1;
    size_t *inferior_frame_pointer = frame_pointer + 1;
    *inferior_frame_pointer = (size_t)(argc - 1);
    size_t *stack_start = inferior_frame_pointer;
    *stack_start = (size_t)win_loader_end;
    LOADER_LOG("frame_pointer: %x\n", frame_pointer);
    LOADER_LOG("stack_start: %x\n", stack_start);
    LOADER_LOG("end func: %x\n", *stack_start);
    LOADER_LOG("------------running program------------\n");

    run_asm(
        (size_t)inferior_frame_pointer, (size_t)stack_start, pe_data.entry_point
    );
}
