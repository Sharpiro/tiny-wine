#include <tiny_c.h>

const char *CONST_DATA = "const data\n";

static char *STATIC_DATA = "static data\n";

void _start(void) {
    tiny_c_printf(CONST_DATA);
    tiny_c_printf(STATIC_DATA);
    tiny_c_exit(0);
}
