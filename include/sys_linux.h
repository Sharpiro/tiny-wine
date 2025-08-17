#pragma once

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

extern int32_t errno_internal;

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

size_t syscall(size_t sys_no, struct SysArgs *sys_args);

int32_t mprotect(void *address, size_t length, int32_t protection);

size_t brk(size_t brk_address);

ssize_t write(int32_t fd, const char *data, size_t length);

int32_t open(const char *path, int32_t flags);

int32_t close(int32_t fd);

size_t read(int32_t fd, void *buf, size_t count);

off_t lseek(int32_t fd, off_t offset, int32_t whence);

void *mmap(
    void *address,
    size_t length,
    int32_t prot,
    int32_t flags,
    int32_t fd,
    off_t offset
);

int32_t munmap(void *address, size_t length);

int32_t getpid(void);

uint32_t getuid();

size_t arch_prctl(size_t code, size_t address);

int32_t uname(struct utsname *name);
