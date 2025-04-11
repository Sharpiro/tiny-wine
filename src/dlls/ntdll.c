#include "ntdll.h"

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

static size_t syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    __asm__("mov rdi, %0" : : "r"(sys_args->param_one));
    __asm__("mov rsi, %0" : : "r"(sys_args->param_two));
    __asm__("mov rdx, %0" : : "r"(sys_args->param_three));
    __asm__("mov rcx, %0" : : "r"(sys_args->param_four));
    __asm__("mov r8, %0" : : "r"(sys_args->param_five));
    __asm__("mov r9, %0" : : "r"(sys_args->param_six));
    __asm__("mov r10, %0" : : "r"(sys_args->param_seven));
    __asm__("mov rax, %0" : : "r"(sys_no));
    __asm__("syscall" : "=r"(result) : :);

    return result;
}

static size_t sys_write(int32_t file_handle, const char *data, size_t size) {
    struct SysArgs args = {
        .param_one = (size_t)file_handle,
        .param_two = (size_t)data,
        .param_three = size,
    };
    return syscall(SYS_write, &args);
}

static size_t sys_exit(int32_t code) {
    struct SysArgs args = {
        .param_one = (size_t)code,
    };
    return syscall(SYS_exit, &args);
}

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

NTSTATUS NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus) {
    if ((int64_t)ProcessHandle != -1) {
        return -1;
    }

    int32_t result = (int32_t)sys_exit(ExitStatus);
    return result;
}

size_t sys_brk(size_t brk) {
    struct SysArgs args = {
        .param_one = brk,
    };
    return syscall(SYS_brk, &args);
}

// @todo: this is a duped func
int32_t mprotect(void *address, size_t length, int32_t protection) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
        .param_three = (size_t)protection,
    };
    int32_t result = (int32_t)syscall(SYS_mprotect, &args);
    return result;
}
