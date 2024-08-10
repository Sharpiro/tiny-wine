#include "../../../tiny_c/tiny_c.h"

ARM32_START_FUNCTION

int main(void) {
    int32_t pid = tiny_c_get_pid();
    tiny_c_printf("pid: %x\n", pid);

    char cwd_buffer[100];
    const char *cwd = tiny_c_get_cwd(cwd_buffer, 100);
    tiny_c_printf("cwd: '%s'\n", cwd);

    uint8_t *buffer = tinyc_malloc(128);
    tiny_c_printf("p: %x\n", buffer);
    tiny_c_printf("n: %x\n", *buffer);
    tinyc_free(buffer);

    buffer = tinyc_malloc(0x800);
    tiny_c_printf("p: %x\n", buffer);
    tiny_c_printf("n: %x\n", *buffer);
    tinyc_free(buffer);

    buffer = tinyc_malloc(0x800);
    tiny_c_printf("p: %x\n", buffer);
    tiny_c_printf("n: %x\n", *buffer);
    tinyc_free(buffer);
}
