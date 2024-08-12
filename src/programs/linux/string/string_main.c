#include "../../../tiny_c/tiny_c.h"

const char *CONST_DATA = "const string\n";

static char *static_str = "static string\n";

const int CONST_ZERO = 0;
const int CONST_ONE = 1;

static int static_zero = 0;
static int static_one = 1;

void _start(void) {
    tiny_c_printf("inline string\n");
    tiny_c_printf(CONST_DATA);
    tiny_c_printf(static_str);
    tiny_c_printf("const zero %x\n", CONST_ZERO);
    tiny_c_printf("static zero %x\n", static_zero);
    tiny_c_printf("const one %x\n", CONST_ONE);
    tiny_c_printf("static one %x\n", static_one);
    tiny_c_exit(0);
}
