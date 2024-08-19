#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define STDOUT 1
#define STDERR 2

#define TINYC_ENOENT -ENOENT;
#define TINYC_EACCES -EACCES

extern int32_t tinyc_errno;

void tiny_c_fprintf(int32_t file_handle, const char *format, ...);
void tiny_c_printf(const char *format, ...);
void tiny_c_exit(int32_t code);
void *tiny_c_mmap(
    size_t address,
    size_t length,
    size_t prot,
    size_t flags,
    int32_t fd,
    size_t offset
);
size_t tiny_c_munmap(size_t address, size_t length);
int32_t tiny_c_open(const char *path, int flags);
void tiny_c_close(int32_t fd);
ssize_t tiny_c_read(int32_t fd, void *buf, size_t count);
int32_t tiny_c_memcmp(const void *__s1, const void *__s2, size_t __n);
int32_t tiny_c_strcmp(const void *__s1, const void *__s2);
int32_t tiny_c_get_pid(void);
char *tiny_c_get_cwd(char *buffer, size_t size);
const char *tinyc_strerror(int32_t err_number);
void *tinyc_malloc_arena(size_t n);
void tinyc_free_arena(void);
off_t tinyc_lseek(int fd, off_t offset, int whence);

#define BAIL(fmt, ...)                                                         \
    tiny_c_fprintf(STDERR, fmt, ##__VA_ARGS__);                                \
    return false;

#ifdef ARM32

#define ARM32_START_FUNCTION                                                   \
    __attribute__((naked)) void _start(void) {                                 \
        __asm__("ldr r0, [sp]\n"                                               \
                "add r1, sp, #4\n"                                             \
                "bl main\n"                                                    \
                "mov r7, #1\n"                                                 \
                "svc #0\n"                                                     \
                :);                                                            \
    }

#define GET_REGISTER(reg)                                                      \
    ({                                                                         \
        size_t result = 0;                                                     \
        __asm__("mov %0, " reg : "=r"(result));                                \
        result;                                                                \
    })

#endif

#ifdef AMD64

#define GET_FRAME_POINTER()                                                    \
    ({                                                                         \
        size_t result = 0;                                                     \
        __asm__("mov rax, rbp" : "=r"(result) : :);                            \
        result;                                                                \
    })

#endif
