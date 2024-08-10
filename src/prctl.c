#define _GNU_SOURCE

#include "prctl.h"
#include "tiny_wine.h"
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

static int dl_iterate_phdr_callback(
    struct dl_phdr_info *info, size_t size, void *data
);
static void init_process_control(void);
static void handle_sig_sys(int sig, siginfo_t *info, void *ucontext_void);

static const char *SYS_CALL_NAMES[61] = {
    [1] = "WRITE",
    [60] = "EXIT",
};
static const size_t SYS_CALL_TABLE_SIZE =
    sizeof(SYS_CALL_NAMES) / sizeof(char *);

static char SYS_CALL_NUM_DISPLAY[16] = {};

static uint8_t block_status = SYSCALL_DISPATCH_FILTER_BLOCK;

static void *prctl_address = NULL;
static size_t prctl_size = 0;

void init_prctl(void) {
    struct sigaction sys_action = {
        .sa_sigaction = handle_sig_sys,
        .sa_flags = SA_SIGINFO,
    };
    sigaction(SIGSYS, &sys_action, NULL);

    dl_iterate_phdr(dl_iterate_phdr_callback, NULL);
    if (prctl_address == NULL) {
        fprintf(stderr, "no good\n");
        exit(EXIT_FAILURE);
    }
    printf("prctl_address: %p\n", prctl_address);
    printf("prctl_size: %p\n", (void *)prctl_size);

    init_process_control();
}

static int dl_iterate_phdr_callback(struct dl_phdr_info *info, size_t, void *) {
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
        printf(
            "\ncall: %s, (%jd, %jd, %jd, %jd, %jd, %jd)\n",
            code_display,
            rdi,
            rsi,
            rdx,
            rcx,
            r8,
            r9
        );
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
static void init_process_control(void) {
    printf("try init prctl\n");

    if (prctl(
            PR_SET_SYSCALL_USER_DISPATCH,
            PR_SYS_DISPATCH_ON,
            prctl_address,
            prctl_size,
            &block_status
        )) {
        perror("prctl failed for libc");
        exit(-1);
    };
    printf("init prctl success\n");
}
