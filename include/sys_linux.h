#pragma once

#include "macros.h"
#include <stddef.h>
#include <stdint.h>
#include <types_linux.h>

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

#define MAP_FAILED ((void *)-1)

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char __domainname[65];
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

EXPORTABLE size_t syscall(size_t sys_no, struct SysArgs *sys_args);

EXPORTABLE __attribute__((noreturn)) void sys_exit(int32_t code);

EXPORTABLE size_t
sys_mprotect(void *address, size_t length, int32_t protection);

EXPORTABLE size_t sys_brk(size_t brk_address);

EXPORTABLE size_t sys_write(int32_t fd, const char *data, size_t length);

EXPORTABLE size_t sys_open(const char *path, int32_t flags);

EXPORTABLE size_t sys_close(int32_t fd);

EXPORTABLE size_t sys_read(int32_t fd, void *buf, size_t count);

EXPORTABLE size_t sys_lseek(int32_t fd, off_t offset, int32_t whence);

EXPORTABLE size_t sys_mmap(
    void *address,
    size_t length,
    int32_t prot,
    int32_t flags,
    int32_t fd,
    off_t offset
);

EXPORTABLE size_t sys_munmap(void *address, size_t length);

EXPORTABLE size_t sys_getpid(void);

EXPORTABLE size_t sys_getcwd(char *buffer, size_t size);

EXPORTABLE size_t sys_getuid();

EXPORTABLE size_t sys_arch_prctl(size_t code, size_t address);

EXPORTABLE size_t sys_uname(struct utsname *name);
