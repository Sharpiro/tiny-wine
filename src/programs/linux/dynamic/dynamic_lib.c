#include "../../../tiny_c/tiny_c.h"
#include <fcntl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#define READ_SIZE 0x1000

extern char **environ;

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

        size_t exponent = tiny_c_pow(10, (data_len - i - 1));
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

struct Split {
    size_t start;
    size_t end;
};

bool string_split(
    const char *str,
    size_t str_len,
    char split_char,
    struct Split **split_entries,
    size_t *split_entries_len
) {
    const int MAX_SPLIT_ENTRIES = 100;

    *split_entries =
        tinyc_malloc_arena(sizeof(struct Split) * MAX_SPLIT_ENTRIES);
    size_t split_index = 0;
    size_t split_start = 0;
    for (size_t i = 0; i < str_len; i++) {
        const char curr_char = str[i];
        if (curr_char == split_char || i + 1 == str_len) {
            (*split_entries)[split_index++] = (struct Split){
                .start = split_start,
                .end = i,
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
    struct Split *lines;
    size_t lines_len;
    if (!string_split(passwd_file, passwd_file_len, '\n', &lines, &lines_len)) {
        tinyc_errno = ENOENT;
        return NULL;
    }

    for (size_t i = 0; i < lines_len; i++) {
        struct Split *curr_line = &lines[i];
        char *line_ptr = passwd_file + curr_line->start;
        size_t line_len = curr_line->end - curr_line->start;
        struct Split *split_entries;
        size_t split_entries_len;
        if (!string_split(
                line_ptr, line_len, ':', &split_entries, &split_entries_len
            )) {
            tinyc_errno = ENOENT;
            return NULL;
        }
        if (split_entries_len < 7) {
            tinyc_errno = ENOENT;
            return NULL;
        }

        struct Split *uid_split = &split_entries[2];
        size_t uid_str_len = uid_split->end - uid_split->start;
        char *uid_str = passwd_file + curr_line->start + uid_split->start;
        size_t parsed_uid = (size_t)atol_len(uid_str, uid_str_len);
        if (parsed_uid == uid) {
            struct Split *username_split = &split_entries[0];
            size_t username_len = username_split->end - username_split->start;
            char *username_file_ptr = passwd_file + curr_line->start;
            char *username = tinyc_malloc_arena(username_len + 1);
            // @todo: could create 'alloc' func to avoid this line
            username[username_len] = 0;
            memcpy(username, username_file_ptr, username_len);
            struct passwd *passwdx = tinyc_malloc_arena(READ_SIZE);
            *passwdx = (struct passwd){
                .pw_name = username,
            };
            return passwdx;
        }
    }

    return NULL;
}
