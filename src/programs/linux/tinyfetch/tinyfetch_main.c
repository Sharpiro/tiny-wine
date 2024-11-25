#include "../../../tiny_c/tiny_c.h"
#include <fcntl.h>
#include <pwd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

bool read_to_string(const char *path, char **content);

int main(int argc, char **argv) {
    if (argc > 1 && tiny_c_strcmp(argv[1], "--extra") == 0) {
        /* PID */
        int32_t pid = tiny_c_get_pid();
        tiny_c_printf("PID: %d\n", pid);

        /* CWD */
        char *cwd_buffer = tinyc_malloc_arena(0x100);
        if (cwd_buffer == NULL) {
            EXIT("malloc failed");
        }
        const char *cwd = tiny_c_get_cwd(cwd_buffer, 100);
        tiny_c_printf("CWD: %s\n", cwd);

        /* Memory regions */
        char *maps_buffer = tinyc_malloc_arena(0x1000);
        if (!read_to_string("/proc/self/maps", &maps_buffer)) {
            EXIT("read failed");
        }
        tiny_c_printf("Mapped address regions:\n%s\n", maps_buffer);
    }

    /* Unix name */
    struct utsname uname;
    if (tinyc_uname(&uname) == -1) {
        EXIT("uname failed");
    }

    /* User */
    uid_t uid = tinyc_getuid();
    struct passwd *user_info = getpwuid(uid);
    if (user_info == NULL) {
        EXIT("getpwuid failed: %d\n", tinyc_errno);
    }
    tiny_c_printf("%s@%s\n", user_info->pw_name, uname.nodename);
    tiny_c_printf("--------------\n");

    /* OS */
    char *os_release_buffer = tinyc_malloc_arena(0x1000);
    if (!read_to_string("/etc/os-release", &os_release_buffer)) {
        EXIT("read failed");
    }

    const char PRETTY_NAME_KEY[] = "PRETTY_NAME";
    const int PRETTY_NAME_KEY_LEN = sizeof(PRETTY_NAME_KEY) - 1;
    const char *pretty_name_start =
        strstr(os_release_buffer, PRETTY_NAME_KEY) + PRETTY_NAME_KEY_LEN + 2;
    ;
    const char *pretty_name_end = strchr(pretty_name_start, '"');
    char *pretty_name = tinyc_malloc_arena(0x100);
    if (pretty_name == NULL) {
        EXIT("malloc failed");
    }

    memcpy(
        pretty_name,
        pretty_name_start,
        (size_t)(pretty_name_end - pretty_name_start)
    );
    tiny_c_printf("OS: %s %s\n", pretty_name, uname.machine);

    /* Kernel */
    tiny_c_printf("Kernel: %s\n", uname.release);

    /* Uptime */
    char *uptime;
    if (!read_to_string("/proc/uptime", &uptime)) {
        EXIT("read failed");
    }

    char *uptime_end = strchr(uptime, '.');
    *uptime_end = 0;
    double uptime_total_seconds = (double)atol(uptime);
    double uptime_days = uptime_total_seconds / 60 / 60 / 24;
    double uptime_hours = (uptime_days - (double)(size_t)uptime_days) * 24;
    double uptime_minutes = (uptime_hours - (double)(size_t)uptime_hours) * 60;
    tiny_c_printf(
        "Uptime: %d days, %d hours, %d minutes\n",
        (size_t)uptime_days,
        (size_t)uptime_hours,
        (size_t)uptime_minutes
    );

    /* Shell */
    tiny_c_printf("Shell: %s\n", user_info->pw_shell);

    tinyc_free_arena();
}
