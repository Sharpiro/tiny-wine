#include <dlls/twlibc.h>
#include <macros.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef LINUX

#include <sys_linux.h>

#define BRK brk

#else

#include <dlls/ntdll.h>
#include <dlls/twlibc_win_proxy.h>

#define BRK brk_win

#endif

EXPORTABLE int32_t errno = 0;

static size_t heap_start = 0;
static size_t heap_end = 0;
static size_t heap_index = 0;

#define FILE_INTERNAL_LIST_SIZE 50

static _FileInternal FILE_INTERNAL_LIST[FILE_INTERNAL_LIST_SIZE] = {
    {.fileno_lazy_maybe = 0, .fileno = 0},
    {.fileno_lazy_maybe = 1, .fileno = 1},
    {.fileno_lazy_maybe = 2, .fileno = 2},
};

EXPORTABLE void DllMainCRTStartup(void) {
}

/**
 * Set n bytes of s to c.
 */
EXPORTABLE void *memset(void *s_buffer, int32_t c_value, size_t n_count) {
    for (size_t i = 0; i < n_count; i++) {
        ((uint8_t *)s_buffer)[i] = (uint8_t)c_value;
    }
    return s_buffer;
}

/**
 * Copy n bytes of src to dest. Pointers can't overlap.
 */
EXPORTABLE void *memcpy(
    void *restrict dest, const void *restrict src, size_t n
) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
    }
    return dest;
}

EXPORTABLE size_t strlen(const char *data) {
    size_t len = 0;
    for (size_t i = 0; true; i++) {
        if (data[i] == 0) {
            break;
        }
        len++;
    }
    return len;
}

EXPORTABLE int32_t strcmp(const char *buffer_a, const char *buffer_b) {
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

EXPORTABLE void *__iob_func() {
    return FILE_INTERNAL_LIST;
}

EXPORTABLE int32_t _fileno(FILE *stream) {
    _FileInternal *internal_file = (_FileInternal *)stream;
    return internal_file->fileno_lazy_maybe;
}

EXPORTABLE FILE *fopen(const char *path, const char *mode) {
    if (strcmp(mode, "r") != 0) {
        errno = EACCES;
        return NULL;
    }

    int32_t fd = open(path, O_RDONLY);
    if (fd == -1 || fd >= FILE_INTERNAL_LIST_SIZE) {
        return NULL;
    }

    FILE_INTERNAL_LIST[fd] = (_FileInternal){
        .fileno_lazy_maybe = fd,
        .fileno = fd,
    };

    FILE *fp = (FILE *)&FILE_INTERNAL_LIST[fd];
    return fp;
}

EXPORTABLE size_t fread(void *buffer, size_t size, size_t count, FILE *file) {
    int32_t fd = fileno(file);
    size_t read_len = read(fd, buffer, size * count);
    return read_len;
}

EXPORTABLE int32_t fseek(FILE *file, int64_t offset, int32_t whence) {
    int32_t fd = fileno(file);
    int result = (int32_t)lseek(fd, offset, whence);
    if (result == -1) {
        return result;
    }
    return 0;
}

EXPORTABLE int32_t fclose(FILE *file) {
    int32_t fd = fileno(file);
    return close(fd);
}

EXPORTABLE void fputs(const char *data, FILE *stream) {
    int32_t file_no = fileno(stream);
    if (data == NULL) {
        const char NULL_STRING[] = "(null)";
        write(file_no, NULL_STRING, sizeof(NULL_STRING) - 1);
        return;
    }

    size_t str_len = strlen(data);
    write(file_no, data, str_len);
}

EXPORTABLE int32_t puts(const char *data) {
    fputs(data, stdout);
    fputs("\n", stdout);
    return 0;
}

EXPORTABLE double pow(double x, double y) {
    double product = 1;
    for (size_t i = 0; i < (size_t)y; i++) {
        product *= x;
    }
    return product;
}

static void print_number_hex(FILE *stream, size_t num) {
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

    int32_t file_no = fileno(stream);
    write(file_no, (char *)num_buffer, buffer_index);
}

static void print_number_decimal(FILE *stream, size_t num) {
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
    size_t writegth = num_start == 0 ? 1 : MAX_DIGITS - num_start;
    int32_t file_no = fileno(stream);
    write(file_no, (char *)print_start, writegth);
}

struct PrintItem {
    size_t start;
    size_t length;
    char formatter;
    bool is_large_formatter;
};

static void fprintf_internal(
    FILE *stream, const char *format, va_list var_args
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

    int32_t file_no = fileno(stream);
    for (size_t i = 0; i < print_items_len; i++) {
        struct PrintItem print_item = print_items[i];
        switch (print_item.formatter) {
        case 0x00: {
            const char *data = format + print_item.start;
            write(file_no, data, print_item.length);
            break;
        }
        case 's': {
            char *data = va_arg(var_args, char *);
            fputs(data, stream);
            break;
        }
        case 'c': {
            char data = va_arg(var_args, char);
            write(file_no, &data, 1);
            break;
        }
        case 'p':
        case 'x': {
            size_t data = print_item.is_large_formatter
                ? va_arg(var_args, uint64_t)
                : va_arg(var_args, uint32_t);
            print_number_hex(stream, data);
            break;
        }
        case 'd': {
            size_t data = print_item.is_large_formatter
                ? va_arg(var_args, uint64_t)
                : va_arg(var_args, uint32_t);
            print_number_decimal(stream, data);
            break;
        }
        default: {
            fputs("<unknown>", stream);
            break;
        }
        }
    }

    va_end(var_args);
}

EXPORTABLE int32_t printf(const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(stdout, format, var_args);
    va_end(var_args);
    return 0;
}

EXPORTABLE int32_t fprintf(
    [[maybe_unused]] FILE *__restrict stream,
    [[maybe_unused]] const char *__restrict format,
    ...
) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(stream, format, var_args);
    va_end(var_args);
    return 0;
}

// @note: leaks memory
EXPORTABLE void *malloc(size_t n) {
    const size_t PAGE_SIZE = 0x1000;

    if (heap_start == 0) {
        heap_start = BRK(0);
        heap_end = heap_start;
        heap_index = heap_start;
    }
    if (heap_index + n > heap_end) {
        size_t extend_size = PAGE_SIZE * (n / PAGE_SIZE) + PAGE_SIZE;
        heap_end = BRK(heap_end + extend_size);
    }

    if (heap_end <= heap_start) {
        return NULL;
    }

    void *address = (void *)heap_index;
    heap_index += n;

    return address;
}

/* Wide C-string length */
EXPORTABLE size_t wcslen(const wchar_t *s) {
    size_t length = 0;
    for (size_t i = 0; true; i++) {
        if (s[i] == 0) {
            break;
        }
        length += 1;
    }
    return length;
}

/**
 * Locate character in string.
 * @return pointer to char or pointer to string's null terminator if not found.
 */
char *strchrnul(const char *string, int c) {
    size_t string_len = strlen(string);
    for (size_t i = 0; i < string_len; i++) {
        if (string[i] == c) {
            return (char *)(string + i);
        }
    }
    return (char *)(string + string_len);
}

EXPORTABLE void calloc() {
    fprintf(stderr, "calloc unimplemented\n");
    exit(42);
}

EXPORTABLE void free() {
    fprintf(stderr, "free unimplemented\n");
    exit(42);
}

EXPORTABLE void strncmp() {
    fprintf(stderr, "strncmp unimplemented\n");
    exit(42);
}

EXPORTABLE int32_t vfprintf(
    [[maybe_unused]] FILE *__restrict stream,
    [[maybe_unused]] const char *__restrict format,
    [[maybe_unused]] __gnuc_va_list arg
) {
    fprintf_internal(stream, format, arg);
    return 0;
}

EXPORTABLE int32_t fputc(int32_t c, FILE *stream) {
    char c_char = (char)c;
    int32_t file_no = fileno(stream);
    if (!write(file_no, &c_char, 1)) {
        return -1;
    }

    return c;
}

EXPORTABLE size_t fwrite(
    [[maybe_unused]] const char *__restrict ptr,
    [[maybe_unused]] size_t size,
    [[maybe_unused]] size_t n,
    [[maybe_unused]] FILE *__restrict stream
) {
    if (size != 1) {
        fprintf(stderr, "unsupported fwrite size\n");
        exit(42);
    }

    for (size_t i = 0; i < n; i++) {
        char c = ptr[i];
        fputc(c, stream);
    }

    return size * n;
}

EXPORTABLE void localeconv() {
    fprintf(stderr, "localeconv unimplemented\n");
    exit(42);
}

EXPORTABLE char *strerror(int32_t errno) {
    switch (errno) {
    case EPERM:
        return "Operation not permitted";
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

#ifdef LINUX

void exit(int32_t code) {
    struct SysArgs args = {.param_one = (size_t)code};
    syscall(SYS_exit, &args);
}

void abort() {
    struct SysArgs args = {.param_one = 3};
    syscall(SYS_exit, &args);
}

#else

void exit(int32_t exit_code) {
    NtTerminateProcess((HANDLE)-1, exit_code);
}

void abort() {
    NtTerminateProcess((HANDLE)-1, 3);
}

#endif
