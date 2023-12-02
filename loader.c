#include <stdint.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#define STDOUT 1
#define NO_LIBC 1

struct SysArgs {
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
};

static void tiny_c_syscall(uint64_t sys_no, struct SysArgs *sys_args) {
    asm("mov rdi, %0" : : "r"(sys_args->rdi));
    asm("mov rsi, %0" : : "r"(sys_args->rsi));
    asm("mov rdx, %0" : : "r"(sys_args->rdx));
    asm("mov rcx, %0" : : "r"(sys_args->rcx));
    asm("mov r8, %0" : : "r"(sys_args->r8));
    asm("mov r9, %0" : : "r"(sys_args->r9));
    asm("mov rax, %0" : : "r"(sys_no));
    asm("syscall");
}

static void tiny_c_puts(const char *data, int size) {
    struct SysArgs args = {
        .rdi = STDOUT,
        .rsi = (uint64_t)data,
        .rdx = size,
    };
    tiny_c_syscall(SYS_write, &args);

    args = (struct SysArgs){
        .rdi = STDOUT,
        .rsi = (uint64_t) "\n",
        .rdx = 1,
    };
    tiny_c_syscall(SYS_write, &args);
}

static void tiny_c_put_number(uint64_t num) {
    const char *HEX_CHARS = "0123456789abcdef";
    char num_buffer[32] = {0};
    num_buffer[8] = '\n';

    for (int i = 0; i < 8; i++) {
        num_buffer[i] = HEX_CHARS[15 - i];
    }

    struct SysArgs args = {
        .rdi = STDOUT,
        .rsi = (uint64_t)num_buffer,
        .rdx = 9,
    };
    tiny_c_syscall(SYS_write, &args);
}

static void tiny_c_exit(int code) {
    struct SysArgs args = {.rdi = code};
    tiny_c_syscall(SYS_exit, &args);
}

static void tiny_c_stat(const char *path) {
    struct stat stat_temp;
    struct SysArgs args = {
        .rdi = (uint64_t)path,
        .rsi = (uint64_t)&stat_temp,
    };
    tiny_c_syscall(SYS_stat, &args);
}

#if NO_LIBC
void _start(void) {
    tiny_c_puts("test", 4);
    tiny_c_stat("todo.md");
    tiny_c_put_number(42);
    tiny_c_exit(0);
}
#endif

#if !NO_LIBC
int main(void) {
    tiny_c_puts("test", 4);
    tiny_c_exit(0);
}
#endif
