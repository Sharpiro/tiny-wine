#include <stddef.h>
#include <stdint.h>
#include <sys_linux.h>

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

void sys_exit(int32_t code) {
    struct SysArgs args = {
        .param_one = (size_t)code,
    };
    syscall(SYS_exit, &args);
}

size_t sys_brk(size_t brk) {
    struct SysArgs args = {
        .param_one = brk,
    };
    return syscall(SYS_brk, &args);
}

size_t sys_write(int32_t fd, const char *data, size_t length) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)data,
        .param_three = length,
    };
    size_t result = syscall(SYS_write, &args);
    return result;
}

size_t sys_mmap(
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
    size_t result = syscall(SYS_mmap, &args);
    return result;
}

size_t sys_open(const char *path, int32_t flags) {
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)flags,
    };
    size_t result = syscall(SYS_open, &args);
    return result;
}

size_t sys_close(int32_t fd) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
    };
    return syscall(SYS_close, &args);
}

size_t sys_read(int32_t fd, void *buf, size_t count) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)buf,
        .param_three = count,
    };
    return syscall(SYS_read, &args);
}

size_t sys_getpid(void) {
    struct SysArgs args = {0};
    return syscall(SYS_getpid, &args);
}

size_t sys_getcwd(char *buffer, size_t size) {
    struct SysArgs args = {
        .param_one = (size_t)buffer,
        .param_two = size,
    };
    size_t result = syscall(SYS_getcwd, &args);
    return result;
}

size_t sys_uname(struct utsname *name) {
    struct SysArgs args = {
        .param_one = (size_t)name,
    };
    size_t result = syscall(SYS_uname, &args);
    return result;
}

size_t sys_getuid() {
    struct SysArgs args = {};
    size_t result = syscall(SYS_getuid, &args);
    return result;
}

size_t sys_lseek(int32_t fd, off_t offset, int32_t whence) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)offset,
        .param_three = (size_t)whence,
    };
    size_t result = syscall(SYS_lseek, &args);
    return result;
}

size_t sys_munmap(void *address, size_t length) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
    };
    size_t result = syscall(SYS_munmap, &args);
    return result;
}

size_t sys_mprotect(void *address, size_t length, int32_t protection) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
        .param_three = (size_t)protection,
    };
    size_t result = syscall(SYS_mprotect, &args);
    return result;
}

size_t sys_arch_prctl(size_t code, size_t address) {
    struct SysArgs args = {
        .param_one = code,
        .param_two = address,
    };
    size_t result = syscall(SYS_arch_prctl, &args);
    return result;
}
