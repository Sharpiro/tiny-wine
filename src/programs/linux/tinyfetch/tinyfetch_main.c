#include "../../../tiny_c/tiny_c.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>

ARM32_START_FUNCTION

extern size_t tinyc_heap_start;
extern size_t tinyc_heap_end;
extern size_t tinyc_heap_index;

#define READ_SIZE 0x1000

bool read_to_string(const char *path, char **content) {
    char *buffer = tinyc_malloc_arena(READ_SIZE);
    if (buffer == NULL) {
        BAIL("malloc failed");
    }

    int32_t fd = tiny_c_open(path, O_RDONLY);
    tiny_c_read(fd, buffer, READ_SIZE);
    tiny_c_close(fd);
    *content = buffer;

    return true;
}

int main(void) {
    int32_t pid = tiny_c_get_pid();
    tiny_c_printf("pid: %x\n", pid);

    char *cwd_buffer = tinyc_malloc_arena(0x100);
    if (cwd_buffer == NULL) {
        BAIL("malloc failed");
    }
    const char *cwd = tiny_c_get_cwd(cwd_buffer, 100);
    tiny_c_printf("cwd: '%s'\n", cwd);

    char *maps_buffer = tinyc_malloc_arena(0x1000);
    if (!read_to_string("/proc/self/maps", &maps_buffer)) {
        BAIL("kernel failed");
    }
    tiny_c_printf("Mapped address regions:\n");
    tiny_c_printf("%s\n", maps_buffer);

    char *kernel;
    if (!read_to_string("/proc/sys/kernel/osrelease", &kernel)) {
        BAIL("read failed");
    }
    tiny_c_printf("Kernel: %s\n", kernel);

    char *uptime;
    if (!read_to_string("/proc/uptime", &uptime)) {
        BAIL("read failed");
    }
    tiny_c_printf("Uptime: %s\n", uptime);
    // size_t x = (size_t)atol("1");
    // tiny_c_printf("atol: %x\n", x);

    tinyc_free_arena();
}
