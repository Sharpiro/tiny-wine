#include <dlls/twlibc.h>
#include <pwd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys_linux.h>

bool read_to_string(const char *path, char **content);

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--extra") == 0) {
        /* PID */
        int32_t pid = getpid();
        printf("PID: %d\n", pid);

        /* CWD */
        char *cwd_buffer = malloc(0x100);
        if (cwd_buffer == NULL) {
            EXIT("malloc failed");
        }
        const char *cwd = getcwd(cwd_buffer, 100);
        printf("CWD: %s\n", cwd);

        /* Memory regions */
        char *maps_buffer = malloc(0x1000);
        if (!read_to_string("/proc/self/maps", &maps_buffer)) {
            EXIT("read failed");
        }
        printf("Mapped address regions:\n%s\n", maps_buffer);
    }

    /* Unix name */
    struct utsname system_name;
    if (uname(&system_name) == -1) {
        EXIT("uname failed");
    }

    /* User */
    uid_t uid = getuid();
    struct passwd *user_info = getpwuid(uid);
    if (user_info == NULL) {
        EXIT("getpwuid failed: %d\n", errno);
    }
    printf("%s@%s\n", user_info->pw_name, system_name.nodename);
    printf("--------------\n");

    /* OS */
    char *os_release_buffer = malloc(0x1000);
    if (!read_to_string("/etc/os-release", &os_release_buffer)) {
        EXIT("read failed");
    }

    const char PRETTY_NAME_KEY[] = "PRETTY_NAME";
    const int PRETTY_NAME_KEY_LEN = sizeof(PRETTY_NAME_KEY) - 1;
    const char *pretty_name_start =
        strstr(os_release_buffer, PRETTY_NAME_KEY) + PRETTY_NAME_KEY_LEN + 2;
    ;
    const char *pretty_name_end = strchr(pretty_name_start, '"');
    char *pretty_name = malloc(0x100);
    if (pretty_name == NULL) {
        EXIT("malloc failed");
    }

    memcpy(
        pretty_name,
        pretty_name_start,
        (size_t)(pretty_name_end - pretty_name_start)
    );
    printf("OS: %s %s\n", pretty_name, system_name.machine);

    /* Kernel */
    printf("Kernel: %s\n", system_name.release);

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
    printf(
        "Uptime: %zd days, %zd hours, %zd minutes\n",
        (size_t)uptime_days,
        (size_t)uptime_hours,
        (size_t)uptime_minutes
    );

    /* Shell */
    printf("Shell: %s\n", user_info->pw_shell);
}
