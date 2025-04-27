#include <dlls/ntdll.h>
#include <sys_linux.h>

int32_t errno = 0;

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

    int32_t result = (int32_t)write(linux_file_handle, buffer, length);
    return result;
}

NTSTATUS
NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus) {
    if ((int64_t)ProcessHandle != -1) {
        return -1;
    }
    struct SysArgs args = {.param_one = (size_t)ExitStatus};
    syscall(SYS_exit, &args);
    return 0;
}

size_t brk_win(size_t brk_address) {
    return brk(brk_address);
}

int32_t mprotect_win(void *address, size_t length, int32_t protection) {
    return mprotect(address, length, protection);
}
