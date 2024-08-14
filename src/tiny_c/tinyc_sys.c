#include "tinyc_sys.h"
#include <sys/syscall.h>

extern size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args);

size_t tinyc_sys_brk(size_t brk) {
    struct SysArgs args = {
        .param_one = brk,
    };
    size_t result = tiny_c_syscall(SYS_brk, &args);
    return result;
}

off_t tinyc_sys_lseek(uint32_t fd, off_t offset, uint32_t whence) {
    struct SysArgs args = {
        .param_one = fd,
        .param_two = (size_t)offset,
        .param_three = whence,
    };
    size_t result = tiny_c_syscall(SYS_lseek, &args);
    off_t result_offset = (off_t)result;
    return result_offset;
}
