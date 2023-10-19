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

#define PROGRAM "test_program/test.exe"

void do_asm(u64 value) {
    asm volatile("jmp %0;"
                 :            // output %1
                 : "r"(value) // input %0
    );
    // asm volatile("mov rax, %0;"
    //              "add rax, 0x01;"
    //              :
    //              : "r"(value) // Input operand (value)
    // );
}

int main() {
    struct stat file_stat;
    stat(PROGRAM, &file_stat);
    printf("size: %ld\n", file_stat.st_size);

    // u8 *buffer = malloc(file_stat.st_size);
    size_t size = 0x3000; // 1 page
    // this will be mapped somewhere between /lib/x86_64-linux-gnu/ld-2.15.so
    // and stack (see note and memory map below)
    void *malloc_buffer = malloc(0x6000);
    void *buffer = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // void *buffer = mmap(NULL, (size_t)1024, PROT_READ | PROT_WRITE |
    // PROT_EXEC,
    //                     MAP_PRIVATE | MAP_ANON, -1, 0);
    printf("malloc ptr %p\n", malloc_buffer);
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

    // mprotect(buffer, size, PROT_READ | PROT_EXEC);
    // printf("mprotect success\n");

    FILE *file = fopen(PROGRAM, "r");
    fread(buffer, 1, file_stat.st_size, file);
    printf("read success\n");
    // fclose(file);

    u8 *offset_buffer = buffer + 0x1000;
    // u8 *offset_buffer = buffer;
    for (int i = 0; i < 0x25; i++) {
        printf("0x%02X, ", offset_buffer[i]);
    }
    printf("\n");

    // u64 value = 0xffffffffff;
    u64 value = (u64)offset_buffer;
    // u64 result;

    // asm volatile("jmp %0;"
    //              : "=r"(result) // Output operand (result)
    //              : "r"(value)   // Input operand (value)
    // );
    do_asm(value);
    // asm volatile("mov rax, %0;"
    //              : "=r"(result) // Output operand (result)
    //              : "r"(value)   // Input operand (value)
    // );
    // asm volatile("mov %1, %0;"
    //              "add rax, 0x01;"
    //              "push rax;"
    //              "mov rax, 0x00;"
    //              "pop rax;"
    //              "add rax, 0x01;"
    //              //  "jmp 0x01;"
    //              : "=r"(result) // Output operand (result)
    //              : "r"(value)   // Input operand (value)
    // );

    // asm(".intel_syntax noprefix");
    // asm volatile("mov eax, 12");
    // asm volatile("mov eax, $1;" // Move 10 into the output operand (%0)
    //              "add $5, %0;"  // Add 5 to the output operand (%0)
    //              : "=r"(result) // Output operand (result)
    //              :              // No input operands
    //              :              // No clobbered registers
    // );

    // while (true) {
    //     // printf("sleep...\n");
    //     sleep(5);
    // }

    // printf("\nresult: %lx\n", result);

    // free(buffer);
}
