#include <elf.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syscall.h>
#include <ucontext.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

#define PROGRAM_SPACE_SIZE 0x200000
#define PROGRAM_ADDRESS_START 0x400000

void run_asm(u64 value, uint64_t program_entry);
void init_signals();

uint8_t block_status = SYSCALL_DISPATCH_FILTER_BLOCK;

void init_process_control_64_bit() {
    printf("try init prctl\n");

    if (prctl(PR_SET_SYSCALL_USER_DISPATCH, PR_SYS_DISPATCH_ON, 0x7ffff7def000,
              0x156000, &block_status)) {
        perror("prctl failed for libc");
        exit(-1);
    };
    printf("init prctl success\n");

    // asm volatile("mov rax, 0xff00;"
    //              "syscall");
    printf("inline asm done\n");
}

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

    init_signals();
    init_process_control_64_bit();

    // while (true) {
    //     sleep(1);
    // }

    printf("starting child program...\n\n");
    run_asm(stack_start, header->e_entry);
    printf("child program complete\n");

    int error = munmap(program_map_buffer, PROGRAM_SPACE_SIZE);
    if (error) {
        printf("munmap failed\n");
        return error;
    }
}

// void handle_sig_int(int sig, siginfo_t *info, void *ucontext) {
//     printf("SIGINT received, %d, %d, %d\n", sig, info->si_signo,
//     info->si_code); exit(0);
// }

void handle_sig_sys(int sig, siginfo_t *info, void *ucontext) {
    //    0x401153:    mov    eax,0x3c
    printf("SIGSYS received, code: %d, signo: %d (%d), sys: %x\n",
           info->si_code, info->si_signo, sig,
           info->_sifields._sigsys._syscall);

    if (info->_sifields._sigsys._syscall == SYS_exit) {
        printf("intercepted exit\n");
        exit(0);
    }

    // int x = getcontext(ucontext);
    ucontext_t *ucontext_typed = ucontext;
    // int64_t x = ucontext_typed->uc_mcontext.gregs[REG_R14];
    printf("try passing syscall through...\n");
    syscall(info->_sifields._sigsys._syscall);
    // printf("sig sys received\n");
}

void init_signals() {
    // struct sigaction sig_int_action = {.sa_sigaction = handle_sig_int};
    // sigaction(SIGINT, &sig_int_action, NULL);

    struct sigaction sys_action = {
        .sa_sigaction = handle_sig_sys,
        .sa_flags = SA_SIGINFO,
    };
    sigaction(SIGSYS, &sys_action, NULL);
}

void run_asm(uint64_t stack_start, uint64_t program_entry) {
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
