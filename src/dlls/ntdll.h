#include "win_type.h"
#include <stddef.h>

#define SYS_write 0x01
#define SYS_mprotect 0x0a
#define SYS_brk 0x0c
#define SYS_exit 0x3c

struct SysArgs {
    size_t param_one;
    size_t param_two;
    size_t param_three;
    size_t param_four;
    size_t param_five;
    size_t param_six;
    size_t param_seven;
};

int32_t NtWriteFile(
    HANDLE file_handle,
    [[maybe_unused]] HANDLE event,
    [[maybe_unused]] PVOID apc_routine,
    [[maybe_unused]] PVOID apc_context,
    [[maybe_unused]] PIO_STATUS_BLOCK io_status_block,
    PVOID buffer,
    ULONG length,
    [[maybe_unused]] PLARGE_INTEGER byte_offset,
    [[maybe_unused]] PULONG key
);

NTSTATUS NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);

size_t sys_brk(size_t brk);

size_t mprotect(size_t address, size_t length, size_t protection);

int large_ntdll(int a, int b, int c, int d, int e, int f, int g, int h);
