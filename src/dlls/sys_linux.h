#pragma once

#include "linux_types.h"
#include <stddef.h>
#include <stdint.h>

#define SYS_read 0
#define SYS_write 1
#define SYS_open 2
#define SYS_close 3
#define SYS_lseek 8
#define SYS_mmap 9
#define SYS_mprotect 10
#define SYS_munmap 11
#define SYS_brk 12
#define SYS_getpid 39
#define SYS_exit 60
#define SYS_uname 63
#define SYS_getcwd 79
#define SYS_getuid 102
#define SYS_arch_prctl 158

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

struct SysArgs {
    size_t param_one;
    size_t param_two;
    size_t param_three;
    size_t param_four;
    size_t param_five;
    size_t param_six;
    size_t param_seven;
};

size_t syscall(size_t sys_no, struct SysArgs *sys_args);

int32_t mprotect(void *address, size_t length, int32_t protection);

size_t brk(size_t brk_address);

ssize_t write(int32_t fd, const char *data, size_t length);

void exit(int32_t exit_code);
