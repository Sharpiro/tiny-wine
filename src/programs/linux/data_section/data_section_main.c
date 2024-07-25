#include <tiny_c.h>

static char *data = "Hello, Tiny Wine!\n";

void _start(void) {
    tiny_c_printf(data);
    tiny_c_exit(0);
}
