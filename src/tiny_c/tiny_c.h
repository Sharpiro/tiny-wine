#pragma once

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/utsname.h>

#define STDOUT 1
#define STDERR 2

#define TINYC_ENOENT -ENOENT;
#define TINYC_EACCES -EACCES

extern int32_t tinyc_errno;

size_t tiny_c_pow(double x, double y);

void tinyc_fputs(const char *data, int32_t file_handle);

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

int32_t tiny_c_munmap(void *address, size_t length);

int32_t tiny_c_mprotect(void *address, size_t length, int32_t protection);

int32_t tiny_c_open(const char *path, int flags);

void tiny_c_close(int32_t fd);

ssize_t tiny_c_read(int32_t fd, void *buf, size_t count);

int32_t tiny_c_memcmp(const void *__s1, const void *__s2, size_t __n);

int32_t tiny_c_strcmp(const char *__s1, const char *__s2);

int32_t tiny_c_get_pid(void);

char *tiny_c_get_cwd(char *buffer, size_t size);

const char *tinyc_strerror(int32_t err_number);

void *malloc(size_t n);

off_t tinyc_lseek(int32_t fd, off_t offset, int whence);

uint32_t divmod(uint32_t numerator, uint32_t denominator);

int32_t tinyc_uname(struct utsname *uname);

uid_t getuid(void);

off_t tinyc_lseek(int32_t fd, off_t offset, int32_t whence);

#define BAIL(fmt, ...)                                                         \
    tiny_c_fprintf(STDERR, fmt, ##__VA_ARGS__);                                \
    return false

#define EXIT(fmt, ...)                                                         \
    tiny_c_fprintf(STDERR, fmt, ##__VA_ARGS__);                                \
    tiny_c_exit(1)

size_t strlen(const char *data);
