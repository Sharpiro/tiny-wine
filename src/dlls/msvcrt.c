#include "msvcrt.h"
#include <stddef.h>
#include <stdint.h>

void DllMainCRTStartup(void) {
}

size_t strlen(const char *data) {
    if (data == NULL) {
        return 0;
    }

    size_t len = 0;
    for (size_t i = 0; true; i++) {
        if (data[i] == 0) {
            break;
        }
        len++;
    }
    return len;
}

void puts(const char *data) {
    size_t data_len = strlen(data);
    NtWriteFile(
        (HANDLE)-11,
        NULL,
        NULL,
        NULL,
        NULL,
        (PVOID)data,
        (ULONG)data_len,
        NULL,
        NULL
    );
    NtWriteFile((HANDLE)-11, NULL, NULL, NULL, NULL, "\n", 1, NULL, NULL);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn"

void exit(int32_t exit_code) {
    NtTerminateProcess((HANDLE)-1, exit_code);
}

#pragma clang diagnostic pop
