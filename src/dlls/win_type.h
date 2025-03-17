#include <stddef.h>
#include <stdint.h>

typedef uint8_t BYTE;
typedef int16_t SHORT;
typedef uint16_t WORD, USHORT;
typedef int32_t LONG, NTSTATUS;
typedef uint32_t DWORD, ULONG;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG, ULONG_PTR;
typedef void *HANDLE, *PVOID, *LPVOID;
typedef uint32_t *PULONG, *PDWORD;
typedef size_t SIZE_T;
typedef wchar_t WCHAR;
typedef WCHAR *PWSTR;

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
