#include <dlls/ntdll.h>
#include <dlls/twlibc_win_proxy.h>

#define STDOUT 1
#define STDERR 2

int32_t open(const char *path, int32_t flags) {
    return open_win(path, flags);
}

int32_t close(int32_t fd) {
    return close_win(fd);
}

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
