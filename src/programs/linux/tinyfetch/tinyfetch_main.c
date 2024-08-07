#include "../../../tiny_c/tiny_c.h"

ARM32_START_FUNCTION

int main(void) {
    char buffer[100];
    const char *cwd = tiny_c_get_cwd(buffer, 100);
    if (cwd == NULL) {
        tiny_c_printf("cwd: null\n", cwd);
        return -1;
    }
    tiny_c_printf("cwd: '%s'\n", cwd);
}
