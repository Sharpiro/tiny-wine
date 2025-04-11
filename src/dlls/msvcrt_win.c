#include "macros.h"
#include "msvcrt.h"
#include "ntdll.h"
#include <stddef.h>

#define STDOUT 1
#define STDERR 2

bool print_len(FILE *file_handle, const char *data, size_t length) {
    int32_t file_no = _fileno(file_handle);
    HANDLE win_handle;
    if (file_no == STDOUT) {
        win_handle = (HANDLE)-11;
    } else if (file_no == STDERR) {
        win_handle = (HANDLE)-12;
    } else {
        return false;
    }

    if (NtWriteFile(
            win_handle,
            NULL,
            NULL,
            NULL,
            NULL,
            (PVOID)data,
            (ULONG)length,
            NULL,
            NULL
        ) == -1) {
        return false;
    }

    return true;
}

EXPORTABLE void exit(int32_t exit_code) {
    NtTerminateProcess((HANDLE)-1, exit_code);
}

EXPORTABLE void abort() {
    NtTerminateProcess((HANDLE)-1, 3);
}
