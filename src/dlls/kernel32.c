#include "ntdll.h"
#include <stdbool.h>
#include <stddef.h>

#define PAGE_EXECUTE_READWRITE 0x40

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
    }
    return dest;
}

void DllMainCRTStartup(void) {
}

void DeleteCriticalSection() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

void EnterCriticalSection() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

int32_t GetLastError() {
    return 42;
}

void GetStartupInfoA() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

void InitializeCriticalSection() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

void LeaveCriticalSection() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

void SetUnhandledExceptionFilter() {
}

void Sleep() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

void TlsGetValue() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

/*
 * Update memory region protection
 */
bool VirtualProtect(
    [[maybe_unused]] const LPVOID lpAddress,
    [[maybe_unused]] SIZE_T dwSize,
    [[maybe_unused]] DWORD flNewProtect,
    [[maybe_unused]] PDWORD lpflOldProtect
) {
    int32_t protection;
    if (flNewProtect == PAGE_EXECUTE_READWRITE) {
        protection = 7;
    } else {
        return false;
    }

    *lpflOldProtect = 0xff;
    mprotect(lpAddress, dwSize, protection);
    return true;
}

typedef struct _MEMORY_BASIC_INFORMATION {
    PVOID BaseAddress;
    PVOID AllocationBase;
    DWORD AllocationProtect;
    WORD PartitionId;
    SIZE_T RegionSize;
    DWORD State;
    DWORD Protect;
    DWORD Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

/*
 * Query memory regions.
 * Fake implementation that just returns the pointer it was given.
 */
size_t VirtualQuery(
    [[maybe_unused]] PVOID lpAddress,
    [[maybe_unused]] PMEMORY_BASIC_INFORMATION lpBuffer,
    [[maybe_unused]] size_t dwLength
) {
    if ((size_t)lpAddress % 0x1000 != 0) {
        return 0;
    }

    *lpBuffer = (MEMORY_BASIC_INFORMATION){
        .BaseAddress = lpAddress,
        .RegionSize = 0x01,
    };
    return sizeof(MEMORY_BASIC_INFORMATION);
}

void IsDBCSLeadByteEx() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

void MultiByteToWideChar() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}

void WideCharToMultiByte() {
    __asm__("int 3\n");
    NtTerminateProcess((HANDLE)-1, 42);
}
