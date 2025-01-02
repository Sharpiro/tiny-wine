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

void puts([[maybe_unused]] const char *data) {
    // size_t data_len = strlen(data);
    // sys_write(1, data, data_len);
    // sys_write(1, "\n", 1);
    // NtWriteFile(
    //     (HANDLE)-11,
    //     NULL,
    //     NULL,
    //     NULL,
    //     NULL,
    //     (PVOID)data,
    //     (ULONG)data_len,
    //     NULL,
    //     NULL
    // );
    // NtWriteFile((HANDLE)-11, NULL, NULL, NULL, NULL, "\n", 1, NULL, NULL);

    NtWriteFile((HANDLE)-11, NULL, NULL, NULL, NULL, "9\n", 2, NULL, NULL);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn"

void exit(int32_t exit_code) {
    NtTerminateProcess((HANDLE)-1, exit_code);
}

#pragma clang diagnostic pop

size_t add_many_msvcrt(
    [[maybe_unused]] size_t one,
    [[maybe_unused]] size_t two,
    [[maybe_unused]] size_t three,
    [[maybe_unused]] size_t four,
    [[maybe_unused]] size_t five,
    [[maybe_unused]] size_t six,
    [[maybe_unused]] size_t seven,
    [[maybe_unused]] size_t eight
) {
    size_t result =
        add_many_ntdll(one, two, three, four, five, six, seven, eight);
    return result;
}
