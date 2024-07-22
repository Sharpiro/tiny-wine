#include "tiny_c.h"
#include <elf.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

static void run_asm(uint8_t *stack_start, size_t program_entry) {
    asm volatile(
        // set stack pointer first
        "mov sp, %[stack_start]\n"

        // clear registers
        "mov r0, #0\n"
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
        : [stack_start] "r"(stack_start)
        :);

    // jump to program
    asm volatile("bx %[program_entry]\n"
                 :
                 : [program_entry] "r"(program_entry)
                 :);
}

#endif

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
    uint8_t *frame_pointer = (uint8_t *)GET_REGISTER("fp");
    size_t argc = *(size_t *)(frame_pointer + sizeof(size_t));
    if (argc < 2) {
        tiny_c_printf("Filename required\n", argc);
        tiny_c_exit(-1);
        return;
    }

    char *filename = *(char **)(frame_pointer + sizeof(size_t) * 3);

    tiny_c_printf("args: %x\n", argc);
    tiny_c_printf("filename: %s\n", filename);

    const size_t ADDRESS = 0x10000;

    if (tiny_c_munmap(ADDRESS, 0x1000)) {
        tiny_c_printf("munmap of self failed\n");
        tiny_c_exit(1);
    }

    ssize_t fd = tiny_c_open(filename);
    if (fd < 0) {
        tiny_c_printf("file not found\n");
        tiny_c_exit(1);
    }
    tiny_c_printf("fd: %x\n", fd);

    // @todo: map data section
    uint8_t *addr =
        tiny_c_mmap(ADDRESS, 0x200000, PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE, fd, 0);
    tiny_c_fclose(fd);
    if (addr == MAP_FAILED) {
        tiny_c_printf("map failed\n");
        tiny_c_exit(1);
    }

    ELF_HEADER *header = (ELF_HEADER *)addr;
    tiny_c_printf("program entry: %x\n", header->e_entry);
    tiny_c_printf("map address: %x\n", (size_t)addr);

    // uint8_t *stack_start = frame_pointer + 0x08;
    uint8_t *stack_start = frame_pointer;
    tiny_c_printf("frame_pointer: %x\n", frame_pointer);
    tiny_c_printf("stack_start: %x\n", stack_start);
    tiny_c_printf("running program...\n");
    run_asm(stack_start, header->e_entry);
}
#endif

#if !NO_LIBC
int main(void) {}
#endif
