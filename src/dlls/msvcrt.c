#include "msvcrt.h"
#include <stddef.h>
#include <stdint.h>

void DllMainCRTStartup(void) {
}

void puts([[maybe_unused]] const char *data) {
    const char *DATA = "my libc puts\n";
    sys_write(1, DATA, 13);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn"

void exit(int32_t exit_code) {
    sys_exit(exit_code);
}

#pragma clang diagnostic pop
