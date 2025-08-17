#pragma once

#include <macros.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types_linux.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
#pragma clang diagnostic ignored "-Winvalid-noreturn"
#pragma clang diagnostic ignored "-Wdll-attribute-on-redeclaration"
#pragma clang diagnostic ignored "-Wbuiltin-requires-header"

#define EPERM 1   /* Operation not permitted */
#define ENOENT 2  /* No such file or directory */
#define EAGAIN 11 /* Try again */
#define EACCES 13 /* Permission denied */
#define EEXIST 17 /* File exists */
#define EINVAL 22 /* Invalid argument */
#define ERANGE 34 /* Math result not representable */

#define SEEK_SET 0 /* Seek from beginning of file.  */
#define O_RDONLY 00

#define PROT_READ 0x1                /* Page can be read.  */
#define PROT_WRITE 0x2               /* Page can be written.  */
#define PROT_EXEC 0x4                /* Page can be executed.  */
#define MAP_PRIVATE 0x02             /* Changes are private.  */
#define MAP_ANONYMOUS 0x20           /* Don't use a file.  */
#define MAP_FIXED 0x10               /* Interpret addr exactly.  */
#define MAP_FIXED_NOREPLACE 0x100000 /* MAP_FIXED but do not unmap */

#define IS_SIGNED(type) (((type) - 1) < 0) ? true : false

#define BAIL(fmt, ...)                                                         \
    fprintf(stderr, fmt, ##__VA_ARGS__);                                       \
    return false

#define EXIT(fmt, ...)                                                         \
    fprintf(stderr, fmt, ##__VA_ARGS__);                                       \
    exit(1)

struct _IO_FILE;

typedef struct _IO_FILE FILE;

typedef struct _FileInternal {
    uint8_t start[16];
    int32_t fileno_lazy_maybe;
    uint32_t p1;
    int32_t fileno;
    uint8_t end[20];
} _FileInternal;

#define stdin ((FILE *)((_FileInternal *)__iob_func()))
#define stdout ((FILE *)((_FileInternal *)__iob_func() + 1))
#define stderr ((FILE *)((_FileInternal *)__iob_func() + 2))

#define MAP_FAILED ((void *)-1)

#ifdef LINUX

extern int32_t errno_internal;
#define errno errno_internal

#else

extern int32_t *_errno();
#define errno (*_errno())

#endif

void *memcpy(void *restrict dest, const void *restrict src, size_t n);

int32_t memcmp(const void *__s1, const void *__s2, size_t __n);

#define fileno(x) _fileno(x)

void *__iob_func();

double pow(double x, double y);

EXPORTABLE void exit(int32_t exit_code);

EXPORTABLE void abort();

char *strchrnul(const char *string, int c);

int32_t printf(const char *format, ...);

int32_t fprintf(FILE *__restrict stream, const char *__restrict __format, ...);

int32_t puts(const char *data);

void *malloc(size_t n);

size_t wcslen(const wchar_t *s);

int32_t _fileno(FILE *stream);

void fputs(const char *data, FILE *file_handle);

char *getcwd(char *buffer, size_t size);

int32_t strcmp(const char *__s1, const char *__s2);

FILE *fopen(const char *path, const char *mode);

size_t fread(void *buffer, size_t size, size_t count, FILE *file);

int32_t fseek(FILE *file, int64_t offset, int32_t whence);

int32_t fclose(FILE *file);

size_t strlen(const char *data);

char *strerror(int32_t err_number);

void *memset(void *s_buffer, int32_t c_value, size_t n_count);
