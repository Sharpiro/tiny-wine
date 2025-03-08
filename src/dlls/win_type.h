#include <stddef.h>
#include <stdint.h>

typedef int16_t WORD;
typedef int32_t LONG, NTSTATUS;
typedef uint32_t DWORD, ULONG;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG, ULONG_PTR;
typedef void *HANDLE, *PVOID, *LPVOID;
typedef uint32_t *PULONG, *PDWORD;
typedef size_t SIZE_T;

typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } DUMMYSTRUCTNAME;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;
    struct {
        DWORD LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;
    LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } DUMMYUNIONNAME;

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
