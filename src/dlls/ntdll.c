#include <dlls/ntdll.h>
#include <sys_linux.h>

#define STDOUT 1
#define STDERR 2

void DllMainCRTStartup(void) {
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"

void *memset(void *s_buffer, int c_value, size_t n_count) {
    for (size_t i = 0; i < n_count; i++) {
        ((uint8_t *)s_buffer)[i] = (uint8_t)c_value;
    }
    return s_buffer;
}

#pragma clang diagnostic pop

NTSTATUS NtWriteFile(
    HANDLE file_handle,
    [[maybe_unused]] HANDLE event,
    [[maybe_unused]] PVOID apc_routine,
    [[maybe_unused]] PVOID apc_context,
    [[maybe_unused]] PIO_STATUS_BLOCK io_status_block,
    PVOID buffer,
    ULONG length,
    [[maybe_unused]] PLARGE_INTEGER byte_offset,
    [[maybe_unused]] PULONG key
) {
    int32_t linux_file_handle;
    if ((int64_t)file_handle == -11) {
        linux_file_handle = 1;
    } else if ((int64_t)file_handle == -12) {
        linux_file_handle = 2;
    } else {
        return -1;
    }

    int32_t result = (int32_t)sys_write(linux_file_handle, buffer, length);
    return result;
}

ssize_t write(int32_t fd, const char *data, size_t length) {
    HANDLE win_handle;
    if (fd == STDOUT) {
        win_handle = (HANDLE)-11;
    } else if (fd == STDERR) {
        win_handle = (HANDLE)-12;
    } else {
        sys_exit(3);
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

NTSTATUS
NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus) {
    if ((int64_t)ProcessHandle != -1) {
        return -1;
    }
    sys_exit((int32_t)ExitStatus);
    return 0;
}

size_t brk(size_t brk_address) {
    return sys_brk(brk_address);
}

// @todo

int32_t mprotect(void *address, size_t length, int32_t protection) {
    return sys_mprotect(address, length, protection);
}

int32_t open(const char *path, int32_t flags) {
    return sys_open(path, flags);
}

size_t read(int32_t fd, void *buf, size_t count) {
    return sys_read(fd, buf, count);
}

off_t lseek(int32_t fd, off_t offset, int whence) {
    return sys_lseek(fd, offset, whence);
}

int32_t close(int32_t fd) {
    return sys_close(fd);
}

int32_t ntdll_large_params_ntdll(
    int32_t one,
    int32_t two,
    int32_t three,
    int32_t four,
    int32_t five,
    int32_t six,
    int32_t seven,
    int32_t eight
) {
    return one + two + three + four + five + six + seven + eight;
}
