#include "sys_linux.h"
#include "msvcrt.h"
#include <stddef.h>
#include <stdint.h>

size_t syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    __asm__("mov rdi, %0" : : "r"(sys_args->param_one));
    __asm__("mov rsi, %0" : : "r"(sys_args->param_two));
    __asm__("mov rdx, %0" : : "r"(sys_args->param_three));
    __asm__("mov rcx, %0" : : "r"(sys_args->param_four));
    __asm__("mov r8, %0" : : "r"(sys_args->param_five));
    __asm__("mov r9, %0" : : "r"(sys_args->param_six));
    __asm__("mov r10, %0" : : "r"(sys_args->param_seven));
    __asm__("mov rax, %0" : : "r"(sys_no));
    __asm__("syscall" : "=r"(result) : :);

    return result;
}

size_t brk(size_t brk) {
    struct SysArgs args = {
        .param_one = brk,
    };
    return syscall(SYS_brk, &args);
}

ssize_t write(int32_t fd, const char *data, size_t length) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)data,
        .param_three = length,
    };
    syscall(SYS_write, &args);
    return (ssize_t)length;
}

void *mmap(
    void *address,
    size_t length,
    int32_t prot,
    int32_t flags,
    int32_t fd,
    off_t offset
) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
        .param_three = (size_t)prot,
        .param_seven =
            (size_t)flags, // @note: disrespects x64 calling convention
        .param_five = (size_t)fd,
        .param_six = (size_t)offset,
    };
    ssize_t result = (ssize_t)syscall(SYS_mmap, &args);
    if (result < 0) {
        errno = (int32_t)-result;
        return MAP_FAILED;
    }

    return (void *)result;
}

int32_t open(const char *path, int32_t flags) {
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)flags,
    };
    int32_t result = (int32_t)syscall(SYS_open, &args);
    if (result < 0) {
        errno = -result;
        return -1;
    }

    return result;
}

int32_t close(int32_t fd) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
    };
    return (int32_t)syscall(SYS_close, &args);
}

ssize_t read(int32_t fd, void *buf, size_t count) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)buf,
        .param_three = count,
    };
    return (ssize_t)syscall(SYS_read, &args);
}

int32_t getpid(void) {
    struct SysArgs args = {0};
    return (int32_t)syscall(SYS_getpid, &args);
}

char *getcwd(char *buffer, size_t size) {
    struct SysArgs args = {
        .param_one = (size_t)buffer,
        .param_two = size,
    };
    size_t result = syscall(SYS_getcwd, &args);
    if (result < 1) {
        return NULL;
    }

    return buffer;
}

int32_t uname(struct utsname *name) {
    struct SysArgs args = {
        .param_one = (size_t)name,
    };
    int32_t result = (int32_t)syscall(SYS_uname, &args);
    if (result < 0) {
        errno = -result;
        return -1;
    }

    return result;
}

uid_t getuid() {
    struct SysArgs args = {};
    uid_t result = (uid_t)syscall(SYS_getuid, &args);
    return result;
}

off_t lseek(int32_t fd, off_t offset, int32_t whence) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)offset,
        .param_three = (size_t)whence,
    };
    off_t result = (off_t)syscall(SYS_lseek, &args);
    return result;
}

int32_t munmap(void *address, size_t length) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
    };
    int32_t result = (int32_t)syscall(SYS_munmap, &args);
    if (result < 0) {
        errno = -result;
        return -1;
    }

    return 0;
}

int32_t mprotect(void *address, size_t length, int32_t protection) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
        .param_three = (size_t)protection,
    };
    int32_t result = (int32_t)syscall(SYS_mprotect, &args);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

size_t arch_prctl(size_t code, size_t address) {
    struct SysArgs args = {
        .param_one = code,
        .param_two = address,
    };
    size_t result = syscall(SYS_arch_prctl, &args);
    return result;
}
