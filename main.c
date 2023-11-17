#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

// #define PROGRAM "test_program_asm/test.exe"
#define PROGRAM "test_program_c_linux/test.exe"
#define PROGRAM_SPACE_SIZE 0x200000
// #define PROGRAM_SPACE_SIZE 900000
#define PROGRAM_ADDRESS_START (void *)0x400000
// #define CODE_START_OFFSET 0x1000
// #define CODE_START_OFFSET 0x1010
#define CODE_START_OFFSET 0x1500
// #define CODE_START_OFFSET 0x1160
#define PROGRAM_CODE_START PROGRAM_ADDRESS_START + CODE_START_OFFSET

void run_asm(u64 value);

int main(int, char *argv[]) {
    uint8_t *buffer = malloc(1024);
    if (buffer == NULL) {
        printf("malloc broke\n");
    }
    printf("malloc still works\n");

    struct stat file_stat;
    stat(PROGRAM, &file_stat);
    printf("size: %ld\n", file_stat.st_size);

    size_t MMAP_LEN = PROGRAM_SPACE_SIZE;
    void *program_map_buffer = mmap(PROGRAM_ADDRESS_START, MMAP_LEN,
                                    PROT_READ | PROT_WRITE | PROT_EXEC,
                                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("mmap ptr %p\n", program_map_buffer);

    // void *stack_buffer = mmap(NULL, 0x22000, PROT_READ | PROT_WRITE |
    // PROT_EXEC,
    //                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (program_map_buffer == NULL) {
        printf("map NULL\n");
        return 1;
    }
    if (program_map_buffer == MAP_FAILED) {
        printf("map failed\n");
        return 1;
    }
    printf("mmap success\n");

    FILE *file = fopen(PROGRAM, "r");
    if (!file) {
        printf("file not found\n");
        return 1;
    }

    fread(program_map_buffer, 1, file_stat.st_size, file);
    printf("read success\n");
    fclose(file);

    // u8 *code_buffer = program_map_buffer + CODE_START_OFFSET;
    // for (int i = 0; i < 0x25; i++) {
    //     printf("0x%02X, ", code_buffer[i]);
    // }
    // printf("\n");

    // char *data_buffer = buffer + 0x2000;
    // u64 *instruction_buffer = (u64 *)(code_buffer + 12);
    // *instruction_buffer = (u64)data_buffer;

    // printf("data start: %s\n", data_buffer);

    // printf("sleeping...\n");
    // bool run = true;
    // while (run) {
    //     sleep(1);
    // }
    uint64_t stack_start = (uint64_t)argv - 8;
    printf("stack_start: %lx\n", stack_start);
    printf("starting child program...\n");
    // u64 value = (u64)code_buffer;
    // uint64_t stack_start = (uint64_t)&argc;
    run_asm(stack_start);
    printf("child program complete\n");

    int error = munmap(program_map_buffer, MMAP_LEN);
    if (error) {
        printf("munmap failed\n");
        return error;
    }
}

void run_asm(uint64_t stack_start) {
    asm volatile("mov rbx, 0x00;"
                 // set stack pointer
                 "mov rsp, %[stack_start];"

                 //  clear 'PF' flag
                 "mov r15, 0xff;"
                 "xor r15, 1;"

                 // clear registers
                 "mov rax, 0x00;"
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
                 // @todo: only works with constants
                 "jmp %[start_address];"
                 :
                 : [start_address] "i"(PROGRAM_CODE_START), [stack_start] "r"(
                                                                stack_start));

    // asm("jmp %[start_address]"
    //     : // Output operands would be here.
    //     : // Input operands are here.
    //     [start_address] "i"(PROGRAM_CODE_START));
}
