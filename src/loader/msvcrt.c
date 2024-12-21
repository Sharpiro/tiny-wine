#include "msvcrt.h"
#include <stddef.h>
#include <stdint.h>

void DllMainCRTStartup(void) {
}

void puts([[maybe_unused]] const char *data) {
    const char *DATA = "my libc puts\n";
    write(1, DATA, 8);
}
