#include "../elf_tools.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <tiny_c.h>

#ifdef AMD64

#define ELF_HEADER Elf64_Ehdr

static void run_asm(size_t stack_start, size_t program_entry) {
    asm volatile("mov rbx, 0x00\n"
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
    asm volatile("jmp %[program_entry]\n"
                 :
                 : [program_entry] "r"(program_entry)
                 : "rax");
}

#endif

#ifdef ARM32

ARM32_START_FUNCTION

static void run_asm(size_t frame_start, size_t stack_start,
                    size_t program_entry) {
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
            : "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9",
              "r10");

    __asm__("mov fp, %[frame_start]\n"
            "mov sp, %[stack_start]\n"
            "bx %[program_entry]\n"
            :
            : [frame_start] "r"(frame_start), [stack_start] "r"(stack_start),
              [program_entry] "r"(program_entry)
            :);
}

#endif

int main(int32_t argc, char **argv) {
    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        return -1;
    }

    int32_t null_file_handle = tiny_c_open("/dev/null");
    int32_t log_handle = STDERR;
    if (argc > 2 && tiny_c_strcmp(argv[2], "silent") == 0) {
        log_handle = null_file_handle;
    }

    char *filename = argv[1];

    tiny_c_fprintf(log_handle, "argc: %x\n", argc);
    tiny_c_fprintf(log_handle, "filename: %s\n", filename);

    // @todo: old gcc maps this by default for some reason
    const size_t ADDRESS = 0x10000;
    if (tiny_c_munmap(ADDRESS, 0x1000)) {
        tiny_c_fprintf(STDERR, "munmap of self failed\n");
        return -1;
    }

    ssize_t fd = tiny_c_open(filename);
    if (fd < 0) {
        tiny_c_fprintf(STDERR, "file not found\n");
        return -1;
    }

    // @todo: map data section

    struct ElfData elf_data;
    if (!get_elf_data(fd, &elf_data)) {
        tiny_c_fprintf(STDERR, "error parsing elf data\n");
        return -1;
    }

    tiny_c_fprintf(log_handle, "memory regions: %x\n",
                   elf_data.memory_regions_len);
    tiny_c_fprintf(log_handle, "program entry: %x\n", elf_data.header->e_entry);

    for (size_t i = 0; i < elf_data.memory_regions_len; i++) {
        struct MemoryRegion *memory_region = &elf_data.memory_regions[i];
        size_t memory_region_len = memory_region->end - memory_region->start;
        // @todo: mem region test?
        size_t prot_read = (memory_region->permissions & 4) >> 2;
        size_t prot_write = memory_region->permissions & 2;
        size_t prot_execute = (memory_region->permissions & 1) << 2;
        size_t map_protection = prot_read | prot_write | prot_execute;
        uint8_t *addr = tiny_c_mmap(memory_region->start, memory_region_len,
                                    map_protection, MAP_PRIVATE, fd, 0);
        tiny_c_fprintf(log_handle, "map address: %x\n", (size_t)addr);
        if ((ssize_t)addr < 1) {
            tiny_c_fprintf(STDERR, "map failed\n");
            return -1;
        }
    }

    size_t *frame_pointer = (size_t *)argv - 1;
    size_t *inferior_frame_pointer = frame_pointer + 1;
    *inferior_frame_pointer = (size_t)(argc - 1);
    size_t *stack_start = inferior_frame_pointer;
    tiny_c_fprintf(log_handle, "frame_pointer: %x\n", frame_pointer);
    tiny_c_fprintf(log_handle, "stack_start: %x\n", stack_start);
    tiny_c_fprintf(log_handle, "running program...\n");
    run_asm((size_t)inferior_frame_pointer, (size_t)stack_start,
            elf_data.header->e_entry);
}
