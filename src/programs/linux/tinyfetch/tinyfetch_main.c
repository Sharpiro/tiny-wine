#include "../../../tiny_c/tiny_c.h"
#include <stddef.h>

ARM32_START_FUNCTION

extern size_t tinyc_heap_start;
extern size_t tinyc_heap_end;
extern size_t tinyc_heap_index;

int main(void) {
    int32_t pid = tiny_c_get_pid();
    tiny_c_printf("pid: %x\n", pid);

    char cwd_buffer[100];
    const char *cwd = tiny_c_get_cwd(cwd_buffer, 100);
    tiny_c_printf("cwd: '%s'\n", cwd);

    const int MALLOC_SIZE = 0x1001;
    uint8_t *buffer = tinyc_malloc_arena(MALLOC_SIZE);
    tiny_c_printf("p: %x\n", buffer);
    tiny_c_printf("f: %x\n", *buffer);
    tiny_c_printf("l: %x\n", *(buffer + MALLOC_SIZE - 1));

    buffer = tinyc_malloc_arena(0x800);
    tiny_c_printf("p: %x\n", buffer);
    tiny_c_printf("n: %x\n", *buffer);

    buffer = tinyc_malloc_arena(0x800);
    tiny_c_printf("p: %x\n", buffer);
    tiny_c_printf("n: %x\n", *buffer);

    tinyc_free_arena();
}
