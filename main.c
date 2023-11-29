#include "prctl.h"
#include "tiny_wine.h"
#include <link.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

static void run_asm(u_int64_t value, uint64_t program_entry);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: linker <filename>\n");
        exit(EXIT_FAILURE);
    }

    char *program_path = argv[1];
    struct stat file_stat;
    stat(program_path, &file_stat);
    printf("size: %ld\n", file_stat.st_size);

    void *program_map_buffer = mmap(
        (void *)PROGRAM_ADDRESS_START, PROGRAM_SPACE_SIZE,
        PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("mmap ptr %p\n", program_map_buffer);

    if (program_map_buffer == NULL) {
        printf("map NULL\n");
        return 1;
    }
    if (program_map_buffer == MAP_FAILED) {
        printf("map failed\n");
        return 1;
    }
    printf("mmap success\n");

    FILE *file = fopen(program_path, "r");
    if (!file) {
        printf("file not found\n");
        return 1;
    }

    fread(program_map_buffer, 1, file_stat.st_size, file);
    printf("read success\n");
    fclose(file);

    Elf64_Ehdr *header = program_map_buffer;
    printf("program entry: %lx\n", header->e_entry);

    uint64_t stack_start = (uint64_t)argv - 8;
    printf("stack_start: %lx\n", stack_start);

    if (PRCTL) {
        init_prctl();
    }

    printf("starting child program...\n\n");
    run_asm(stack_start, header->e_entry);
    printf("child program complete\n");

    int error = munmap(program_map_buffer, PROGRAM_SPACE_SIZE);
    if (error) {
        printf("munmap failed\n");
        return error;
    }
}

static void run_asm(uint64_t stack_start, uint64_t program_entry) {
    asm volatile(
        "mov rbx, 0x00;"
        // set stack pointer
        "mov rsp, %[stack_start];"

        //  clear 'PF' flag
        "mov r15, 0xff;"
        "xor r15, 1;"

        // clear registers
        //  "mov rax, 0x00;"
        "mov rcx, 0x00;"
        "mov rdx, 0x00;"
        "mov rsi, 0x00;"
        "mov rdi, 0x00;"
        "mov rbp, 0x00;"
        "mov r8, 0x00;"
        "mov r9, 0x00;"
        "mov r10, 0x00;"
        "mov r11, 0x00;"
        "mov r12, 0x00;"
        "mov r13, 0x00;"
        "mov r14, 0x00;"
        "mov r15, 0x00;"

        // jump to program
        "jmp %[program_entry];"
        :
        : [program_entry] "r"(program_entry), [stack_start] "r"(stack_start));
}
