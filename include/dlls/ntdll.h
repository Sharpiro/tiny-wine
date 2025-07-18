#include <macros.h>
#include <stddef.h>
#include <types_win.h>

EXPORTABLE int32_t NtWriteFile(
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

EXPORTABLE NTSTATUS
NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);

EXPORTABLE size_t brk_win(size_t brk);

EXPORTABLE int32_t
mprotect_win(void *address, size_t length, int32_t protection);

EXPORTABLE int32_t open_win(const char *path, int32_t flags);

EXPORTABLE int32_t close_win(int32_t fd);
