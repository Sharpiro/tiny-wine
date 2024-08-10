#include "tinyc_sys.h"
#include <sys/syscall.h>

extern size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args);

size_t tinyc_sys_brk(size_t brk) {
    struct SysArgs args = {
        .param_one = brk,
    };
    size_t result = tiny_c_syscall(SYS_brk, &args);
    // int32_t err = (int32_t)result;
    // if (err < 1) {
    //     tinyc_errno = -err;
    //     return -1;
    // }

    return result;
}
