#include "tiny_c.h"
#include <elf.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define NO_LIBC 1

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

#define ELF_HEADER Elf32_Ehdr

static void run_asm(size_t *frame_start, size_t *stack_start,
                    size_t program_entry) {
    asm volatile("mov r0, #0\n"
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

    asm volatile(
        "mov fp, %[frame_start]\n"
        "mov sp, %[stack_start]\n"
        "bx %[program_entry]\n"
        :
        : [frame_start] "r"(frame_start), [stack_start] "r"(stack_start),
          [program_entry] "r"(program_entry)
        :);
}

#endif

void _start(void) {
    ssize_t null_file_handle = tiny_c_open("/dev/null");
    ssize_t log_file_handle = STDERR;

    size_t *frame_pointer = (size_t *)GET_REGISTER("fp");
    size_t argc = frame_pointer[1];
    char **argv = (char **)(frame_pointer + 2);
    if (argc < 2) {
        tiny_c_fprintf(STDERR, "Filename required\n", argc);
        tiny_c_exit(-1);
        return;
    }

    if (argc > 2 && tiny_c_strcmp(argv[2], "silent") == 0) {
        log_file_handle = null_file_handle;
    }

    char *filename = argv[1];

    tiny_c_fprintf(log_file_handle, "argc: %x\n", argc);
    tiny_c_fprintf(log_file_handle, "filename: %s\n", filename);

    const size_t ADDRESS = 0x10000;

    if (tiny_c_munmap(ADDRESS, 0x1000)) {
        tiny_c_fprintf(STDERR, "munmap of self failed\n");
        tiny_c_exit(1);
    }

    ssize_t fd = tiny_c_open(filename);
    if (fd < 0) {
        tiny_c_fprintf(STDERR, "file not found\n");
        tiny_c_exit(1);
    }
    tiny_c_fprintf(log_file_handle, "fd: %x\n", fd);

    // @todo: map data section
    uint8_t *addr =
        tiny_c_mmap(ADDRESS, 0x200000, PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE, fd, 0);
    tiny_c_fclose(fd);
    if (addr == NULL || addr == MAP_FAILED) {
        tiny_c_fprintf(STDERR, "map failed\n");
        tiny_c_exit(1);
    }

    const char ELF_MAGIC[] = {0x7f, 'E', 'L', 'F'};

    ELF_HEADER *header = (ELF_HEADER *)addr;
    tiny_c_fprintf(log_file_handle, "program entry: %x\n", header->e_entry);
    tiny_c_fprintf(log_file_handle, "map address: %x\n", (size_t)addr);

    if (tiny_c_memcmp(header->e_ident, ELF_MAGIC, 4)) {
        tiny_c_fprintf(STDERR, "Program type not supported\n");
        tiny_c_exit(1);
    }

    size_t *frame_start = frame_pointer + 1;
    *(frame_start + 1) = argc - 1;
    size_t *stack_start = frame_start + 1;
    tiny_c_fprintf(log_file_handle, "frame_pointer: %x\n", frame_pointer);
    tiny_c_fprintf(log_file_handle, "stack_start: %x\n", stack_start);
    tiny_c_fprintf(log_file_handle, "running program...\n");
    run_asm(frame_start, stack_start, header->e_entry);
}
