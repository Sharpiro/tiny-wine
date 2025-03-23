#include "../../../tiny_c/tiny_c.h"
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define READ_SIZE 0x1000

extern char **environ;

int32_t test_number_bss = 0;
int32_t test_number_data = 12345;
static int32_t test_number_data_internal_ref = 12345;

int32_t get_test_number_data_internal_ref(void) {
    return test_number_data_internal_ref;
}

void set_test_number_data_internal_ref(int32_t val) {
    test_number_data_internal_ref = val;
}

size_t strlen(const char *s) {
    size_t len = 0;
    for (ssize_t i = 0; true; i++) {
        if (s[i] == 0) {
            break;
        }
        len++;
    }

    return len;
}

long atol_len(const char *data, size_t data_len) {
    if (data == NULL) {
        return 0;
    }

    long result = 0;
    for (size_t i = 0; i < data_len; i++) {
        char current_char = data[i];
        ssize_t current_digit = current_char - 0x30;
        if (current_digit < 0 || current_digit > 9) {
            return 0;
        }

        size_t exponent = tiny_c_pow(10, (double)(data_len - i - 1));
        result += current_digit * (ssize_t)exponent;
    }

    return result;
}

long atol(const char *data) {
    size_t data_len = strlen(data);
    return atol_len(data, data_len);
}

char *strchr(const char *s, int c) {
    for (ssize_t i = 0; true; i++) {
        if (s[i] == 0) {
            break;
        }
        if (s[i] == c) {
            return (char *)s + i;
        }
    }

    return NULL;
}

char *strstr(const char *haystack, const char *needle) {
    size_t needle_index = 0;
    for (ssize_t i = 0; true; i++) {
        char haystack_char = haystack[i];
        char needle_char = needle[needle_index];
        if (haystack_char == 0) {
            break;
        }
        if (needle_char == 0) {
            return (char *)haystack + i - needle_index;
        }
        if (haystack_char != needle_char) {
            needle_index = 0;
            continue;
        }

        needle_index++;
    }

    return NULL;
}

int strncmp(const char *buffer_a, const char *buffer_b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        char a = buffer_a[i];
        char b = buffer_b[i];
        if (a == 0 && b == 0) {
            break;
        }
        if (a != b) {
            return a - b;
        }
    }

    return 0;
}

char *getenv(const char *name) {
    for (size_t i = 0; true; i++) {
        char *var = environ[i];
        if (var == NULL) {
            break;
        }
        size_t name_len = strlen(name);
        char *equals_offset = var + name_len;
        if (strncmp(var, name, name_len) == 0 && *equals_offset == '=') {
            return equals_offset + 1;
        }
    }

    return NULL;
}

bool read_to_string(const char *path, char **content) {
    char *buffer = malloc(READ_SIZE);
    if (buffer == NULL) {
        BAIL("malloc failed\n");
    }

    int32_t fd = tiny_c_open(path, O_RDONLY);
    tiny_c_read(fd, buffer, READ_SIZE);
    tiny_c_close(fd);
    *content = buffer;

    return true;
}

struct Split {
    const char *split;
    size_t split_len;
};

bool string_split(
    const char *str,
    size_t str_len,
    char split_char,
    struct Split **split_entries,
    size_t *split_entries_len
) {
    const int MAX_SPLIT_ENTRIES = 100;

    *split_entries = malloc(sizeof(struct Split) * MAX_SPLIT_ENTRIES);
    size_t split_index = 0;
    size_t split_start = 0;
    for (size_t i = 0; i < str_len; i++) {
        const char curr_char = str[i];
        bool is_last_char = i + 1 == str_len;
        if (curr_char == split_char || is_last_char) {
            const char *split = str + split_start;
            size_t split_len =
                is_last_char ? i - split_start + 1 : i - split_start;
            (*split_entries)[split_index++] = (struct Split){
                .split = split,
                .split_len = split_len,
            };
            if (split_index == MAX_SPLIT_ENTRIES) {
                BAIL("max split entries exceeded\n");
            }
            split_start = i + 1;
        }
    }

    *split_entries_len = split_index;
    return true;
}

struct passwd *getpwuid(uid_t uid) {
    char *passwd_file;
    if (!read_to_string("/etc/passwd", &passwd_file)) {
        tinyc_errno = ENOENT;
        return NULL;
    }

    size_t passwd_file_len = strlen(passwd_file);
    struct Split *user_lines;
    size_t user_lines_len;
    if (!string_split(
            passwd_file, passwd_file_len, '\n', &user_lines, &user_lines_len
        )) {
        tinyc_errno = ENOENT;
        return NULL;
    }

    for (size_t i = 0; i < user_lines_len; i++) {
        struct Split *user_line = &user_lines[i];
        struct Split *user_details_split;
        size_t user_details_split_len;
        if (!string_split(
                user_line->split,
                user_line->split_len,
                ':',
                &user_details_split,
                &user_details_split_len
            )) {
            tinyc_errno = ENOENT;
            return NULL;
        }
        if (user_details_split_len < 7) {
            tinyc_errno = ENOENT;
            return NULL;
        }

        struct Split *uid_split = &user_details_split[2];
        size_t parsed_uid =
            (size_t)atol_len(uid_split->split, uid_split->split_len);
        if (parsed_uid == uid) {
            struct Split *username_split = &user_details_split[0];
            char *username = malloc(username_split->split_len + 1);
            memcpy(username, username_split->split, username_split->split_len);
            memset(username + user_line->split_len, 0, 1);

            struct Split *shell_split = &user_details_split[6];
            char *shell = malloc(shell_split->split_len + 1);
            memcpy(shell, shell_split->split, shell_split->split_len);
            memset(shell + shell_split->split_len, 0, 1);

            struct passwd *passwd = malloc(sizeof(struct passwd));
            *passwd = (struct passwd){
                .pw_name = username,
                .pw_shell = shell,
            };
            return passwd;
        }
    }

    return NULL;
}

size_t add_many(
    [[maybe_unused]] size_t one,
    [[maybe_unused]] size_t two,
    [[maybe_unused]] size_t three,
    [[maybe_unused]] size_t four,
    [[maybe_unused]] size_t five,
    [[maybe_unused]] size_t six,
    [[maybe_unused]] size_t seven,
    [[maybe_unused]] size_t eight
) {
    return one + two + three + four + five + six + seven + eight;
}
