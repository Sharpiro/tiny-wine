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

void do_asm(u64 value);

int main() {
    struct stat file_stat;
    stat(PROGRAM, &file_stat);
    printf("size: %ld\n", file_stat.st_size);

    size_t MMAP_LEN = 0x3000;
    void *start = (void *)0x400000;
    void *buffer = mmap(start, MMAP_LEN, PROT_READ | PROT_WRITE | PROT_EXEC,
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

    u8 *code_buffer = buffer + 0x1000;
    for (int i = 0; i < 0x25; i++) {
        printf("0x%02X, ", code_buffer[i]);
    }
    printf("\n");

    // char *data_buffer = buffer + 0x2000;
    // u64 *instruction_buffer = (u64 *)(code_buffer + 12);
    // *instruction_buffer = (u64)data_buffer;

    // printf("data start: %s\n", data_buffer);

    printf("starting child program...\n");
    u64 value = (u64)code_buffer;
    do_asm(value);
    printf("child program complete\n");

    int error = munmap(buffer, MMAP_LEN);
    if (error) {
        printf("munmap failed\n");
        return error;
    }
}

void do_asm(u64 value) {
    asm volatile("jmp %0;"
                 :            // output %1
                 : "r"(value) // input %0
    );
}
