#include <macros.h>
#include <types_linux.h>
#include <types_win.h>

EXPORTABLE NTSTATUS
NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);

EXPORTABLE size_t brk(size_t brk);

EXPORTABLE int32_t mprotect(void *address, size_t length, int32_t protection);

EXPORTABLE int32_t open(const char *path, int32_t flags);

EXPORTABLE size_t read(int32_t fd, void *buf, size_t count);

EXPORTABLE off_t lseek(int32_t fd, off_t offset, int whence);

EXPORTABLE int32_t close(int32_t fd);

EXPORTABLE ssize_t write(int32_t fd, const char *data, size_t length);

EXPORTABLE int32_t ntdll_large_params_ntdll(
    int32_t one,
    int32_t two,
    int32_t three,
    int32_t four,
    int32_t five,
    int32_t six,
    int32_t seven,
    int32_t eight
);
