#include "msvcrt.h"
#include "sys.h"
#include <stddef.h>
#include <sys/syscall.h>
#include <sys/types.h>

size_t sys_call(size_t sys_no, struct SysArgs *sys_args) {
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

size_t sys_brk(size_t brk) {
    struct SysArgs args = {
        .param_one = brk,
    };
    return sys_call(SYS_brk, &args);
}

bool print_len(FILE *file_handle, const char *data, size_t length) {
    int32_t file_no = fileno(file_handle);
    struct SysArgs args = {
        .param_one = (size_t)file_no,
        .param_two = (size_t)data,
        .param_three = length,
    };
    sys_call(SYS_write, &args);
    return true;
}

void exit(int32_t code) {
    struct SysArgs args = {.param_one = (size_t)code};
    sys_call(SYS_exit, &args);
}

void abort() {
    exit(3);
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
    size_t result = sys_call(SYS_mmap, &args);
    ssize_t err = (ssize_t)result;
    if (err < 1) {
        errno = (int32_t)-err;
        return MAP_FAILED;
    }

    return (void *)result;
}
