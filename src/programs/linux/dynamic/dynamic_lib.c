#include "../../../tiny_c/tiny_c.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

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

long atol(const char *data) {
    if (data == NULL) {
        return 0;
    }

    long result = 0;
    size_t num_len = strlen(data);
    for (size_t i = 0; i < num_len; i++) {
        char current_char = data[i];
        ssize_t current_digit = current_char - 0x30;
        if (current_digit < 0 || current_digit > 9) {
            return 0;
        }

        size_t exponent = tiny_c_pow(10, (num_len - i - 1));
        result += current_digit * (ssize_t)exponent;
    }

    return result;
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
            break;
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
