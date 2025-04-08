#include "tinyc.h"
#include "../loader/log.h"
#include "tinyc_sys.h"
#include <stdarg.h>
#include <string.h>
#include <sys/syscall.h>

int32_t errno = 0;

const int32_t internal_files[] = {0, 1, 2};

static size_t heap_start = 0;
static size_t heap_end = 0;
static size_t heap_index = 0;

int32_t fileno(FILE *stream) {
    int32_t *internal_file = (int32_t *)stream;
    return *internal_file;
}

static bool print_len(FILE *file_handle, const char *data, size_t length) {
    int32_t file_no = fileno(file_handle);
    struct SysArgs args = {
        .param_one = (size_t)file_no,
        .param_two = (size_t)data,
        .param_three = length,
    };
    tinyc_syscall(SYS_write, &args);
    return true;
}

size_t strlen(const char *data) {
    size_t len = 0;
    for (size_t i = 0; true; i++) {
        if (data[i] == 0) {
            break;
        }
        len++;
    }
    return len;
}

void fputs(const char *data, FILE *file_handle) {
    if (data == NULL) {
        const char NULL_STRING[] = "(null)";
        print_len(file_handle, NULL_STRING, sizeof(NULL_STRING) - 1);
        return;
    }

    size_t str_len = strlen(data);
    print_len(file_handle, data, str_len);
}

double pow(double x, double y) {
    double product = 1;
    for (size_t i = 0; i < (size_t)y; i++) {
        product *= x;
    }
    return product;
}

static void print_number_hex(FILE *file_handle, size_t num) {
    const size_t MAX_DIGITS = sizeof(num) * 2;
    const char *NUMBER_CHARS = "0123456789abcdef";

    char num_buffer[32] = {0};

    size_t current_base = (size_t)pow(0x10, MAX_DIGITS - 1);
    size_t buffer_index = 0;
    bool num_start = false;
    for (size_t i = 0; i < MAX_DIGITS; i++) {
        size_t digit = num / current_base;
        size_t digit_value = digit * current_base;
        num -= digit_value;
        current_base >>= 4;
        if (num_start || digit > 0 | i + 1 == MAX_DIGITS) {
            num_start = true;
            num_buffer[buffer_index++] = NUMBER_CHARS[digit];
        }
    }

    print_len(file_handle, (char *)num_buffer, buffer_index);
}

static void print_number_decimal(FILE *file_handle, size_t num) {
    const size_t MAX_DIGITS = sizeof(num) * 2;
    const char *NUMBER_CHARS = "0123456789";

    char num_buffer[32] = {0};
    size_t current_base = (size_t)pow(10, MAX_DIGITS - 1);
    size_t num_start = 0;
    size_t i;
    for (i = 0; i < MAX_DIGITS; i++) {
        size_t digit = num / current_base;
        if (num_start == 0 && digit > 0) {
            num_start = i;
        }
        num_buffer[i] = NUMBER_CHARS[digit];
        size_t digit_value = digit * current_base;
        num -= digit_value;
        current_base /= 10;
    }

    size_t print_start = (size_t)num_buffer + num_start;
    size_t print_len = num_start == 0 ? 1 : MAX_DIGITS - num_start;
    struct SysArgs args = {
        .param_one = (size_t)file_handle,
        .param_two = print_start,
        .param_three = print_len,
    };
    tinyc_syscall(SYS_write, &args);
}

struct PrintItem {
    size_t start;
    size_t length;
    char formatter;
    bool is_large_formatter;
};

static void fprintf_internal(
    FILE *file_handle, const char *format, va_list var_args
) {
    size_t print_items_len = 0;
    struct PrintItem print_items[128] = {0};

    size_t last_print_index = 0;
    size_t i;
    for (i = 0; true; i++) {
        char current_char = format[i];
        if (current_char == 0x00) {
            break;
        }
        if (current_char == '%') {
            struct PrintItem print_item = {
                .start = last_print_index,
                .length = i - last_print_index,
            };
            print_items[print_items_len++] = print_item;

            bool is_large_formatter = false;
            if (format[i + 1] == 'z') {
                is_large_formatter = true;
                i += 1;
            }
            struct PrintItem print_item_formater = {
                .formatter = format[i + 1],
                .is_large_formatter = is_large_formatter,
            };
            print_items[print_items_len++] = print_item_formater;
            i += 1;
            last_print_index = i + 1;
        }
    }

    struct PrintItem print_item = {
        .start = last_print_index,
        .length = i - last_print_index,
    };
    print_items[print_items_len++] = print_item;

    for (size_t i = 0; i < print_items_len; i++) {
        struct PrintItem print_item = print_items[i];
        switch (print_item.formatter) {
        case 0x00: {
            const char *data = format + print_item.start;
            print_len(file_handle, data, print_item.length);
            break;
        }
        case 's': {
            char *data = va_arg(var_args, char *);
            fputs(data, file_handle);
            break;
        }
        case 'c': {
            char data = va_arg(var_args, char);
            print_len(file_handle, &data, 1);
            break;
        }
        case 'p':
        case 'x': {
            size_t data = print_item.is_large_formatter
                ? va_arg(var_args, uint64_t)
                : va_arg(var_args, uint32_t);
            print_number_hex(file_handle, data);
            break;
        }
        case 'd': {
            size_t data = print_item.is_large_formatter
                ? va_arg(var_args, uint64_t)
                : va_arg(var_args, uint32_t);
            print_number_decimal(file_handle, data);
            break;
        }
        default: {
            fputs("<unknown>", file_handle);
            break;
        }
        }
    }

    va_end(var_args);
}

void printf(const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(stdout, format, var_args);
    va_end(var_args);
}

void fprintf(FILE *file_handle, const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(file_handle, format, var_args);
    va_end(var_args);
}

void exit(int32_t code) {
    struct SysArgs args = {.param_one = (size_t)code};
    tinyc_syscall(SYS_exit, &args);
}

int32_t open(const char *path, int32_t flags) {
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)flags,
    };
    int32_t result = (int32_t)tinyc_syscall(SYS_open, &args);
    int32_t err = (int32_t)result;
    if (err < 1) {
        errno = -err;
        return -1;
    }

    return result;
}

int32_t close(int32_t fd) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
    };
    return (int32_t)tinyc_syscall(SYS_close, &args);
}

ssize_t read(int32_t fd, void *buf, size_t count) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)buf,
        .param_three = count,
    };
    return (ssize_t)tinyc_syscall(SYS_read, &args);
}

void *mmap(
    void *address,
    size_t length,
    int32_t prot,
    int32_t flags,
    int32_t fd,
    off_t offset
) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
        .param_three = (size_t)prot,
        .param_seven =
            (size_t)flags, // @note: disrespects x64 calling convention
        .param_five = (size_t)fd,
        .param_six = (size_t)offset,
    };
    size_t result = tinyc_syscall(SYS_mmap, &args);
    ssize_t err = (ssize_t)result;
    if (err < 1) {
        errno = (int32_t)-err;
        return MAP_FAILED;
    }

    return (void *)result;
}

int32_t munmap(void *address, size_t length) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
    };
    size_t result = tinyc_syscall(SYS_munmap, &args);

    return (int32_t)result;
}

int32_t mprotect(void *address, size_t length, int32_t protection) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
        .param_three = (size_t)protection,
    };
    size_t result = tinyc_syscall(SYS_mprotect, &args);
    int32_t err = (int32_t)result;
    if (err < 1) {
        errno = -err;
    }

    return err;
}

int32_t memcmp(const void *buffer_a, const void *buffer_b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        u_int8_t a = ((u_int8_t *)buffer_a)[i];
        u_int8_t b = ((u_int8_t *)buffer_b)[i];
        if (a != b) {
            return a - b;
        }
    }

    return 0;
}

int32_t strcmp(const char *buffer_a, const char *buffer_b) {
    for (size_t i = 0; true; i++) {
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

int32_t getpid(void) {
    struct SysArgs args = {0};
    return (int32_t)tinyc_syscall(SYS_getpid, &args);
}

char *getcwd(char *buffer, size_t size) {
    struct SysArgs args = {
        .param_one = (size_t)buffer,
        .param_two = size,
    };
    size_t result = tinyc_syscall(SYS_getcwd, &args);
    if (result < 1) {
        return NULL;
    }

    return buffer;
}

char *strerror(int32_t err_number) {
    switch (err_number) {
    case ENOENT:
        return "No such file or directory";
    case EAGAIN:
        return "Resource temporarily unavailable";
    case EACCES:
        return "Permission denied";
    case EEXIST:
        return "File exists";
    case EINVAL:
        return "Invalid argument";
    case ERANGE:
        return "Math result not representable";
    default:
        return "Unknown";
    }
}

void *malloc(size_t n) {
    const size_t PAGE_SIZE = 0x1000;

    if (heap_start == 0) {
        heap_start = tinyc_sys_brk(0);
        heap_end = heap_start;
        heap_index = heap_start;
    }
    if (heap_index + n > heap_end) {
        size_t extend_size = PAGE_SIZE * (n / PAGE_SIZE) + PAGE_SIZE;
        heap_end = tinyc_sys_brk(heap_end + extend_size);
        LOGDEBUG("tinyc malloc heap extended to 0x%zx\n", heap_end);
    }

    if (heap_end <= heap_start) {
        return NULL;
    }

    void *address = (void *)heap_index;
    heap_index += n;

    return address;
}

off_t lseek(int32_t fd, off_t offset, int32_t whence) {
    return tinyc_sys_lseek((uint32_t)fd, offset, (uint32_t)whence);
}

int32_t uname(struct utsname *name) {
    int32_t result = (int32_t)tinyc_sys_uname(name);
    if (result < 0) {
        errno = -result;
        return -1;
    }

    return result;
}

uid_t getuid(void) {
    struct SysArgs args = {0};
    uid_t result = (uid_t)tinyc_syscall(SYS_getuid, &args);
    return result;
}

void *memset(void *s_buffer, int c_value, size_t n_count) {
    for (size_t i = 0; i < n_count; i++) {
        ((uint8_t *)s_buffer)[i] = (uint8_t)c_value;
    }
    return s_buffer;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
    }
    return dest;
}

struct LinuxStat {
    int8_t padding[48];
    int32_t st_size;
};

int32_t stat(const char *path, struct LinuxStat *file_stat) {
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)file_stat,
    };
    int32_t result = tinyc_syscall(SYS_stat, &args) == 0 ? 0 : -1;

    return result;
}
