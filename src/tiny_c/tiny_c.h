#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STDOUT 1
#define STDERR 2

void tiny_c_fprintf(size_t file_handle, const char *format, ...);
void tiny_c_printf(const char *format, ...);
void tiny_c_exit(int32_t code);
void *tiny_c_mmap(size_t address, size_t length, size_t prot, size_t flags,
                  size_t fd, size_t offset);
size_t tiny_c_munmap(size_t address, size_t length);
int32_t tiny_c_open(const char *path);
void tiny_c_fclose(size_t fd);
int32_t tiny_c_memcmp(const void *__s1, const void *__s2, size_t __n);
int32_t tiny_c_strcmp(const void *__s1, const void *__s2);

#ifdef ARM32

#define GET_REGISTER(reg)                                                      \
    ({                                                                         \
        size_t result = 0;                                                     \
        asm("mov %0, " reg : "=r"(result));                                    \
        result;                                                                \
    })

#endif

#ifdef AMD64

#define GET_FRAME_POINTER()                                                    \
    ({                                                                         \
        size_t result = 0;                                                     \
        asm("mov rax, rbp" : "=r"(result) : :);                                \
        result;                                                                \
    })

#endif
