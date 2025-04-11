#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/utsname.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
#pragma clang diagnostic ignored "-Winvalid-noreturn"
#pragma clang diagnostic ignored "-Wbuiltin-requires-header"

struct _IO_FILE;

typedef struct _IO_FILE FILE;

extern const int32_t internal_files[];
#define stdin (FILE *)internal_files
#define stdout (FILE *)(internal_files + 1)
#define stderr (FILE *)(internal_files + 2)

#define ENOENT 2  /* No such file or directory */
#define EAGAIN 11 /* Try again */
#define EACCES 13 /* Permission denied */
#define EEXIST 17 /* File exists */
#define EINVAL 22 /* Invalid argument */
#define ERANGE 34 /* Math result not representable */

#define SEEK_SET 0 /* Seek from beginning of file.  */
#define O_RDONLY 00

#define MAP_FAILED ((void *)-1)
#define PROT_READ 0x1                /* Page can be read.  */
#define PROT_WRITE 0x2               /* Page can be written.  */
#define PROT_EXEC 0x4                /* Page can be executed.  */
#define MAP_PRIVATE 0x02             /* Changes are private.  */
#define MAP_ANONYMOUS 0x20           /* Don't use a file.  */
#define MAP_FIXED 0x10               /* Interpret addr exactly.  */
#define MAP_FIXED_NOREPLACE 0x100000 /* MAP_FIXED but do not unmap */

extern int32_t errno;

void *memcpy(void *restrict dest, const void *restrict src, size_t n);

double pow(double x, double y);

void fputs(const char *data, FILE *file_handle);

void fprintf(FILE *file_handle, const char *format, ...);

int32_t printf(const char *format, ...);

void exit(int32_t code);

void *mmap(
    void *address,
    size_t length,
    int32_t prot,
    int32_t flags,
    int32_t fd,
    off_t offset
);

int32_t munmap(void *address, size_t length);

int32_t mprotect(void *address, size_t length, int32_t protection);

int32_t open(const char *path, int flags);

int32_t close(int32_t fd);

ssize_t read(int32_t fd, void *buf, size_t count);

int32_t memcmp(const void *__s1, const void *__s2, size_t __n);

int32_t strcmp(const char *__s1, const char *__s2);

int32_t getpid(void);

char *getcwd(char *buffer, size_t size);

char *strerror(int32_t err_number);

void *malloc(size_t n);

off_t lseek(int32_t fd, off_t offset, int whence);

uint32_t divmod(uint32_t numerator, uint32_t denominator);

int32_t uname(struct utsname *name);

uid_t getuid(void);

off_t lseek(int32_t fd, off_t offset, int32_t whence);

#define BAIL(fmt, ...)                                                         \
    fprintf(stderr, fmt, ##__VA_ARGS__);                                       \
    return false

#define EXIT(fmt, ...)                                                         \
    fprintf(stderr, fmt, ##__VA_ARGS__);                                       \
    exit(1)

size_t strlen(const char *data);
