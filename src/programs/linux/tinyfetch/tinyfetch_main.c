#include "../../../tiny_c/tiny_c.h"
#include <fcntl.h>
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

    uint8_t *buffer = tinyc_malloc_arena(0x1000);
    if (buffer == NULL) {
        BAIL("malloc failed");
    }

    int32_t fd = tiny_c_open("/proc/self/maps", O_RDONLY);
    tiny_c_read(fd, buffer, 0x1000);
    tiny_c_printf("heap start: %x\n", buffer);
    tiny_c_printf("Mapped address spaces:\n");
    tiny_c_printf("%s\n", buffer);

    tinyc_free_arena();
}
