#include "tiny_c.h"
#include <elf.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define NO_LIBC 1

static void run_asm(uint64_t stack_start, uint64_t program_entry) {
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
                 //  "mov rbp, 0x00;"
                 "mov r8, 0x00;"
                 "mov r9, 0x00;"
                 "mov r10, 0x00;"
                 "mov r11, 0x00;"
                 "mov r12, 0x00;"
                 "mov r13, 0x00;"
                 "mov r14, 0x00;"
                 "mov r15, 0x00;"
                 :
                 : [stack_start] "r"(stack_start));

    // jump to program
    asm volatile("jmp %[program_entry];"
                 :
                 : [program_entry] "r"(program_entry)
                 : "rax");
}

#define GET_RBP()                                                              \
    ({                                                                         \
        uint64_t rbp_out = 0;                                                  \
        asm("mov rax, rbp" : "=r"(rbp_out) : :);                               \
        rbp_out;                                                               \
    })

void print_buffer(uint8_t *buffer, size_t length) {

    for (size_t i = 0; i < length; i++) {
        if (i > 0 && i % 2 == 0) {
            tiny_c_printf("\n", buffer[i]);
        }
        tiny_c_printf("%x, ", buffer[i]);
    }
    tiny_c_printf("\n");
}

#if NO_LIBC
void _start(void) {
    const char *FILE_NAME = "test_program_asm/test.exe";
    // const char *FILE_NAME = "test_program_c_linux/test.exe";
    const size_t ADDRESS = 0x400000;

    if (tiny_c_munmap(ADDRESS, 0x1000)) {
        tiny_c_printf("munmap failed");
        tiny_c_exit(1);
    }

    uint64_t fd = tiny_c_fopen(FILE_NAME);
    tiny_c_printf("fd: %x\n", fd);

    // 0xA2000
    uint8_t *addr =
        tiny_c_mmap(ADDRESS, 0x200000, PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE, fd, 0);
    tiny_c_fclose(fd);
    if (addr == MAP_FAILED) {
        tiny_c_printf("map failed\n");
        tiny_c_exit(1);
    }
    if ((uint64_t)addr == 0xfffffffffffffff7) {
        tiny_c_printf("map failed for unknown reason\n");
        tiny_c_exit(1);
    }

    Elf64_Ehdr *header = (Elf64_Ehdr *)addr;
    tiny_c_printf("program entry: %x\n", header->e_entry);
    tiny_c_printf("map address: %x\n", (uint64_t)addr);

    uint64_t rbp = GET_RBP();
    uint64_t stack_start = rbp + 0x08;
    tiny_c_printf("rbp: %x\n", rbp);
    tiny_c_printf("stack_start: %x\n", stack_start);
    tiny_c_printf("running program...\n");
    run_asm(stack_start, header->e_entry);
}
#endif

#if !NO_LIBC
int main(void) {
    char *temp = "hi";
    printf("%s\n", temp);
}
#endif
