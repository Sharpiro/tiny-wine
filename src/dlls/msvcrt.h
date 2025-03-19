#include "ntdll.h"

#define STDOUT 1
#define STDERR 2

#define IS_SIGNED(type) (((type) - 1) < 0) ? true : false

struct _IO_FILE;

typedef struct _IO_FILE FILE;

typedef struct _WinFileInternal {
    uint8_t start[16];
    int32_t fileno_lazy_maybe;
    uint32_t p1;
    int32_t fileno;
    uint8_t end[20];
} _WinFileInternal;

#define stdin ((FILE *)((_WinFileInternal *)__iob_func()))
#define stdout ((FILE *)((_WinFileInternal *)__iob_func() + 1))
#define stderr ((FILE *)((_WinFileInternal *)__iob_func() + 2))

extern char *_acmdln;

void *__iob_func();

double pow(double x, double y);

void exit(int32_t exit_code);

int32_t printf(const char *format, ...);

int32_t fprintf(FILE *__restrict stream, const char *__restrict __format, ...);

int32_t puts(const char *data);

int32_t fileno(FILE *file);

void *malloc(size_t n);

size_t wcslen(const wchar_t *s);
