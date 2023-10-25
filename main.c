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

#define PROGRAM "test_program_asm/test.exe"
// #define PROGRAM "test_program_c_linux/test.exe"
#define PROGRAM_SPACE_SIZE 0x3000
// #define PROGRAM_SPACE_SIZE 900000
#define PROGRAM_ADDRESS_START 0x400000
// #define CODE_START_OFFSET 0x1000
#define CODE_START_OFFSET 0x1010
// #define CODE_START_OFFSET 0x1500

void run_asm(u64 value);

int main() {
    struct stat file_stat;
    stat(PROGRAM, &file_stat);
    printf("size: %ld\n", file_stat.st_size);

    size_t MMAP_LEN = PROGRAM_SPACE_SIZE;
    void *program_address_start = (void *)PROGRAM_ADDRESS_START;
    void *buffer = mmap(program_address_start, MMAP_LEN,
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("mmap ptr %p\n", buffer);

    if (buffer == NULL) {
        printf("map NULL\n");
        return 1;
    }
    if (buffer == MAP_FAILED) {
        printf("map failed\n");
        return 1;
    }
    printf("mmap success\n");

    FILE *file = fopen(PROGRAM, "r");
    if (!file) {
        printf("file not found\n");
        return 1;
    }

    fread(buffer, 1, file_stat.st_size, file);
    printf("read success\n");
    fclose(file);

    u8 *code_buffer = buffer + CODE_START_OFFSET;
    for (int i = 0; i < 0x25; i++) {
        printf("0x%02X, ", code_buffer[i]);
    }
    printf("\n");

    // char *data_buffer = buffer + 0x2000;
    // u64 *instruction_buffer = (u64 *)(code_buffer + 12);
    // *instruction_buffer = (u64)data_buffer;

    // printf("data start: %s\n", data_buffer);

    // printf("sleeping...\n");
    // bool run = true;
    // while (run) {
    //     sleep(1);
    // }
    printf("starting child program...\n");
    u64 value = (u64)code_buffer;
    run_asm(value);
    printf("child program complete\n");

    int error = munmap(buffer, MMAP_LEN);
    if (error) {
        printf("munmap failed\n");
        return error;
    }
}

void run_asm(u64 value) {
    asm volatile("jmp %0;"
                 :            // output %1
                 : "r"(value) // input %0
    );
    // asm volatile("mov rbx, 0x00;"
    //              "mov rcx, 0x00;"
    //              "mov rdx, 0x00;"
    //              "mov rsi, 0x00;"
    //              "mov rdi, 0x00;"
    //              "mov rbp, 0x00;"
    //              "mov rsp, 0x00007fffffffda20;"
    //              "mov r8, 0x00;"
    //              "mov r9, 0x00;"
    //              "mov r10, 0x00;"
    //              "mov r11, 0x00;"
    //              "mov r12, 0x00;"
    //              "mov r13, 0x00;"
    //              "mov r14, 0x00;"
    //              "mov r15, 0x00;"
    //              "jmp %0;"
    //              "mov rax, 0x00;"
    //              :            // output %1
    //              : "r"(value) // input %0
    // );
}
