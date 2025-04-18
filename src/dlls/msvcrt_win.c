#include "macros.h"
#include "msvcrt.h"
#include "ntdll.h"
#include <stddef.h>

#define STDOUT 1
#define STDERR 2

ssize_t write(int32_t fd, const char *data, size_t length) {
    HANDLE win_handle;
    if (fd == STDOUT) {
        win_handle = (HANDLE)-11;
    } else if (fd == STDERR) {
        win_handle = (HANDLE)-12;
    } else {
        return -1;
    }

    return NtWriteFile(
        win_handle,
        NULL,
        NULL,
        NULL,
        NULL,
        (PVOID)data,
        (ULONG)length,
        NULL,
        NULL
    );
}

EXPORTABLE void exit(int32_t exit_code) {
    NtTerminateProcess((HANDLE)-1, exit_code);
}

EXPORTABLE void abort() {
    NtTerminateProcess((HANDLE)-1, 3);
}
