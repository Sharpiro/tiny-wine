#include <tiny_c.h>

const char *CONST_DATA = "const string\n";

void _start(void) {
    tiny_c_printf(CONST_DATA);
    tiny_c_printf("inline string\n");
    tiny_c_exit(0);
}
