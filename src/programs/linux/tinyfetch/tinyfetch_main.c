#include "../../../tiny_c/tiny_c.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define READ_SIZE 0x1000

bool read_to_string(const char *path, char **content) {
    char *buffer = tinyc_malloc_arena(READ_SIZE);
    if (buffer == NULL) {
        BAIL("malloc failed\n");
    }

    int32_t fd = tiny_c_open(path, O_RDONLY);
    tiny_c_read(fd, buffer, READ_SIZE);
    tiny_c_close(fd);
    *content = buffer;

    return true;
}

int main(void) {
    /* PID */
    int32_t pid = tiny_c_get_pid();
    tiny_c_printf("PID: %x\n", pid);

    /* CWD */
    char *cwd_buffer = tinyc_malloc_arena(0x100);
    if (cwd_buffer == NULL) {
        BAIL("malloc failed");
    }
    const char *cwd = tiny_c_get_cwd(cwd_buffer, 100);
    tiny_c_printf("CWD: %s\n", cwd);

    /* Memory regions */
    char *maps_buffer = tinyc_malloc_arena(0x1000);
    if (!read_to_string("/proc/self/maps", &maps_buffer)) {
        BAIL("read failed");
    }
    tiny_c_printf("Mapped address regions:\n%s\n", maps_buffer);

    /* OS */
    char *os_release_buffer = tinyc_malloc_arena(0x1000);
    if (!read_to_string("/etc/os-release", &os_release_buffer)) {
        BAIL("read failed");
    }

    const char PRETTY_NAME_KEY[] = "PRETTY_NAME";
    const int PRETTY_NAME_KEY_LEN = sizeof(PRETTY_NAME_KEY) - 1;
    const char *pretty_name_start =
        strstr(os_release_buffer, PRETTY_NAME_KEY) + PRETTY_NAME_KEY_LEN + 2;
    ;
    const char *pretty_name_end = strchr(pretty_name_start, '"');
    char *pretty_name = tinyc_malloc_arena(0x100);
    if (pretty_name == NULL) {
        BAIL("malloc failed");
    }

    memcpy(
        pretty_name,
        pretty_name_start,
        (size_t)(pretty_name_end - pretty_name_start)
    );
    tiny_c_printf("OS: %s\n", pretty_name);

    /* Kernel */
    char *kernel;
    if (!read_to_string("/proc/sys/kernel/osrelease", &kernel)) {
        BAIL("read failed");
    }
    tiny_c_printf("Kernel: %s", kernel);

    /* Uptime */
    char *uptime;
    if (!read_to_string("/proc/uptime", &uptime)) {
        BAIL("read failed");
    }

    char *uptime_end = strchr(uptime, '.');
    *uptime_end = 0;
    double uptime_total_seconds = (double)atol(uptime);
    double uptime_days = uptime_total_seconds / 60 / 60 / 24;
    double uptime_hours = (uptime_days - (size_t)uptime_days) * 24;
    double uptime_minutes = (uptime_hours - (size_t)uptime_hours) * 60;
    tiny_c_printf(
        "Uptime: %x days, %x hours, %x minutes\n",
        (size_t)uptime_days,
        (size_t)uptime_hours,
        (size_t)uptime_minutes
    );

    // struct utsname x = {0};

    /* Shell */
    const char *shell = getenv("SHELL");
    tiny_c_printf("Shell: %s\n", shell);

    tinyc_free_arena();
}
