#define _GNU_SOURCE

#include <elf.h>
#include <link.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syscall.h>
#include <ucontext.h>
#include <unistd.h>

static const char *SYS_CALL_NAMES[61] = {
    [1] = "WRITE",
    [60] = "EXIT",
};
static const size_t SYS_CALL_TABLE_SIZE =
    sizeof(SYS_CALL_NAMES) / sizeof(char *);

static char SYS_CALL_NUM_DISPLAY[16] = {};

#define PROGRAM_SPACE_SIZE 0x200000
#define PROGRAM_ADDRESS_START 0x400000
#define LOG_SIGSYS 1

static int dl_iterate_phdr_callback(struct dl_phdr_info *info, size_t size,
                                    void *data);
static void init_process_control_64_bit(void);
static void handle_sig_sys(int sig, siginfo_t *info, void *ucontext_void);
static void run_asm(u_int64_t value, uint64_t program_entry);

static uint8_t block_status = SYSCALL_DISPATCH_FILTER_BLOCK;

static void *prctl_address = NULL;
static size_t prctl_size = 0;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: linker <filename>\n");
        exit(EXIT_FAILURE);
    }

    dl_iterate_phdr(dl_iterate_phdr_callback, NULL);
    if (prctl_address == NULL) {
        fprintf(stderr, "no good\n");
        exit(EXIT_FAILURE);
    }
    printf("prctl_address: %p\n", prctl_address);
    printf("prctl_size: %p\n", (void *)prctl_size);

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

    struct sigaction sys_action = {
        .sa_sigaction = handle_sig_sys,
        .sa_flags = SA_SIGINFO,
    };
    sigaction(SIGSYS, &sys_action, NULL);

    init_process_control_64_bit();

    printf("starting child program...\n\n");
    run_asm(stack_start, header->e_entry);
    printf("child program complete\n");

    int error = munmap(program_map_buffer, PROGRAM_SPACE_SIZE);
    if (error) {
        printf("munmap failed\n");
        return error;
    }
}

static int dl_iterate_phdr_callback(struct dl_phdr_info *info, size_t size,
                                    void *data) {
    // @todo: not portable
    if (strcmp(info->dlpi_name, "/lib64/libc.so.6") != 0) {
        return 0;
    }

    for (int j = 0; j < info->dlpi_phnum; j++) {
        const Elf64_Phdr *object_header = &info->dlpi_phdr[j];
        if (object_header->p_type != PT_LOAD ||
            object_header->p_flags != 0x05) {
            continue;
        }

        prctl_address = (void *)(info->dlpi_addr + info->dlpi_phdr[j].p_vaddr);
        prctl_size = info->dlpi_phdr[j].p_memsz;

        break;
    }

    return 0;
}

static void handle_sig_sys(int, siginfo_t *info, void *ucontext_void) {
    uint64_t sys_call_no = info->_sifields._sigsys._syscall;
    char *code_display = NULL;
    if (sys_call_no < SYS_CALL_TABLE_SIZE) {
        code_display = (char *)SYS_CALL_NAMES[sys_call_no];
    }
    if (code_display == NULL) {
        code_display = SYS_CALL_NUM_DISPLAY;
        snprintf(code_display, 16, "%jd", sys_call_no);
    }

    ucontext_t *ucontext = ucontext_void;
    int64_t rax = ucontext->uc_mcontext.gregs[REG_RAX];
    int64_t rdi = ucontext->uc_mcontext.gregs[REG_RDI];
    int64_t rsi = ucontext->uc_mcontext.gregs[REG_RSI];
    int64_t rdx = ucontext->uc_mcontext.gregs[REG_RDX];
    int64_t rcx = ucontext->uc_mcontext.gregs[REG_RCX];
    int64_t r8 = ucontext->uc_mcontext.gregs[REG_R8];
    int64_t r9 = ucontext->uc_mcontext.gregs[REG_R9];
    if (LOG_SIGSYS) {
        printf("\ncall: %s, (%jd, %jd, %jd, %jd, %jd, %jd)\n", code_display,
               rdi, rsi, rdx, rcx, r8, r9);
    }

    if (sys_call_no != (uint64_t)rax) {
        fprintf(stderr, "unexpected syscall");
    }

    // @todo: doesn't handle stack vars
    int64_t result = syscall(rax, rdi, rsi, rdx, rcx, r8, r9);
    if (LOG_SIGSYS) {
        printf("\nresult: %jd\n", result);
    }
}

/**
 * 'libc' memory location on 64 bit.
 * 'vdso' memory location on 32 bit
 */
static void init_process_control_64_bit(void) {
    printf("try init prctl\n");

    if (prctl(PR_SET_SYSCALL_USER_DISPATCH, PR_SYS_DISPATCH_ON, prctl_address,
              prctl_size, &block_status)) {
        perror("prctl failed for libc");
        exit(-1);
    };
    printf("init prctl success\n");
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
