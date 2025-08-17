#include <dlls/ntdll.h>
#include <dlls/twlibc_win_proxy.h>

// @todo: rm this proxy somehow?

#define STDOUT 1
#define STDERR 2

int32_t open(const char *path, int32_t flags) {
    return open_win(path, flags);
}

size_t read(int32_t fd, void *buf, size_t count) {
    return read_win(fd, buf, count);
}
off_t lseek(int32_t fd, off_t offset, int whence) {
    return lseek_win(fd, offset, whence);
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
