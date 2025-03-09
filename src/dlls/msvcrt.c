#include "msvcrt.h"
#include "./export.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
#pragma clang diagnostic ignored "-Winvalid-noreturn"
#pragma clang diagnostic ignored "-Wdll-attribute-on-redeclaration"
#pragma clang diagnostic ignored "-Wbuiltin-requires-header"

static size_t heap_start = 0;
static size_t heap_end = 0;
static size_t heap_index = 0;
static int32_t errno = 0;

static _WinFileInternal WIN_FILE_INTERNAL_LIST[] = {
    {.fileno_lazy_maybe = 0, .fileno = 0},
    {.fileno_lazy_maybe = 1, .fileno = 1},
    {.fileno_lazy_maybe = 2, .fileno = 2},
};

void DllMainCRTStartup(void) {
}

EXPORTABLE char *_acmdln;

EXPORTABLE size_t strlen(const char *data) {
    if (data == NULL) {
        return 0;
    }

    size_t len = 0;
    for (size_t i = 0; true; i++) {
        if (data[i] == 0) {
            break;
        }
        len++;
    }
    return len;
}

static bool print_len(int32_t file_handle, const char *data, size_t length) {
    HANDLE win_handle;
    if (file_handle == STDOUT) {
        win_handle = (HANDLE)-11;
    } else if (file_handle == STDERR) {
        win_handle = (HANDLE)-12;
    } else {
        return false;
    }

    if (NtWriteFile(
            win_handle,
            NULL,
            NULL,
            NULL,
            NULL,
            (PVOID)data,
            (ULONG)length,
            NULL,
            NULL
        ) == -1) {
        return false;
    }

    return true;
}

static void print(int32_t file_handle, const char *data) {
    if (data == NULL) {
        const char NULL_STRING[] = "(null)";
        print_len(file_handle, NULL_STRING, sizeof(NULL_STRING) - 1);
        return;
    }

    size_t str_len = strlen(data);
    print_len(file_handle, data, str_len);
}

EXPORTABLE int32_t puts(const char *data) {
    print(STDOUT, data);
    print(STDOUT, "\n");
    return 0;
}

EXPORTABLE double pow(double x, double y) {
    double product = 1;
    for (size_t i = 0; i < (size_t)y; i++) {
        product *= x;
    }

    return product;
}

static void print_number_hex(int32_t file_handle, size_t num) {
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

static void print_number_decimal(int32_t file_handle, size_t num) {
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
    size_t print_length = num_start == 0 ? 1 : MAX_DIGITS - num_start;
    print_len(file_handle, (char *)print_start, print_length);
}

struct PrintItem {
    size_t start;
    size_t length;
    char formatter;
};

static void fprintf_internal(
    int32_t file_handle, const char *format, va_list var_args
) {
    size_t print_items_index = 0;
    size_t print_items_count = 0;
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
            print_items[print_items_index++] = print_item;

            struct PrintItem print_item_str = {
                .formatter = format[i + 1],
            };
            print_items[print_items_index++] = print_item_str;
            print_items_count += 2;
            i++;
            last_print_index = i + 1;
        }
    }

    struct PrintItem print_item = {
        .start = last_print_index,
        .length = i - last_print_index,
    };
    print_items[print_items_index++] = print_item;
    print_items_count++;

    for (size_t i = 0; i < print_items_count; i++) {
        struct PrintItem print_item = print_items[i];
        switch (print_item.formatter) {
        case 0x00: {
            const char *data = format + print_item.start;
            print_len(file_handle, data, print_item.length);
            break;
        }
        case 's': {
            char *data = va_arg(var_args, char *);
            print(file_handle, data);
            break;
        }
        case 'p':
        case 'x': {
            size_t data = va_arg(var_args, size_t);
            print_number_hex(file_handle, data);
            break;
        }
        case 'd': {
            size_t data = va_arg(var_args, size_t);
            print_number_decimal(file_handle, data);
            break;
        }
        default: {
            print(file_handle, "<unknown>");
            break;
        }
        }
    }

    va_end(var_args);
}

EXPORTABLE void exit(int32_t exit_code) {
    NtTerminateProcess((HANDLE)-1, exit_code);
}

EXPORTABLE int32_t printf(const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(STDOUT, format, var_args);
    va_end(var_args);
    return 0;
}

EXPORTABLE size_t add_many_msvcrt(
    [[maybe_unused]] size_t one,
    [[maybe_unused]] size_t two,
    [[maybe_unused]] size_t three,
    [[maybe_unused]] size_t four,
    [[maybe_unused]] size_t five,
    [[maybe_unused]] size_t six,
    [[maybe_unused]] size_t seven,
    [[maybe_unused]] size_t eight
) {
    size_t result =
        add_many_ntdll(one, two, three, four, five, six, seven, eight);
    return result;
}

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF

EXPORTABLE void *__iob_func(void) {
    return WIN_FILE_INTERNAL_LIST;
}

EXPORTABLE int32_t _fileno(FILE *stream) {
    _WinFileInternal *internal_file = (_WinFileInternal *)stream;
    return internal_file->fileno_lazy_maybe;
}

#endif

// @todo: uses custom fprintf, not stdlib.h fprintf
EXPORTABLE int32_t fprintf(
    [[maybe_unused]] FILE *__restrict stream,
    [[maybe_unused]] const char *__restrict format,
    ...
) {
    int32_t file_no = _fileno(stream);
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(file_no, format, var_args);
    va_end(var_args);
    return 0;
}

EXPORTABLE void __C_specific_handler() {
    fprintf(stderr, "__C_specific_handler unimplemented\n");
    exit(42);
}

EXPORTABLE void __getmainargs() {
    fprintf(stderr, "__getmainarg unimplemented\n");
    exit(42);
}

EXPORTABLE void __initenv() {
    fprintf(stderr, "__initenv unimplemented\n");
    exit(42);
}

EXPORTABLE void __lconv_init() {
    fprintf(stderr, "__lconv_init unimplemented\n");
    exit(42);
}

EXPORTABLE void __set_app_type() {
    fprintf(stderr, "__set_app_type unimplemented\n");
    exit(42);
}

EXPORTABLE void __setusermatherr() {
    fprintf(stderr, "__setusermatherr unimplemented\n");
    exit(42);
}

EXPORTABLE void _amsg_exit() {
    fprintf(stderr, "_amsg_exit unimplemented\n");
    exit(42);
}

EXPORTABLE void _cexit() {
    fprintf(stderr, "_cexit unimplemented\n");
    exit(42);
}

EXPORTABLE void _commode() {
    fprintf(stderr, "_commode unimplemented\n");
    exit(42);
}

EXPORTABLE void _fmode() {
    fprintf(stderr, "_fmode unimplemented\n");
    exit(42);
}

EXPORTABLE void _initterm() {
}

EXPORTABLE void _onexit([[maybe_unused]] void (*func)()) {
}

EXPORTABLE void abort() {
    NtTerminateProcess((HANDLE)-1, 3);
}

EXPORTABLE void calloc() {
    fprintf(stderr, "calloc unimplemented\n");
    exit(42);
}

EXPORTABLE void free() {
    fprintf(stderr, "free unimplemented\n");
    exit(42);
}

// @note: fake malloc that leaks memory
EXPORTABLE void *malloc(size_t n) {
    const size_t PAGE_SIZE = 0x1000;

    if (heap_start == 0) {
        heap_start = sys_brk(0);
        heap_end = heap_start;
        heap_index = heap_start;
    }
    if (heap_index + n > heap_end) {
        size_t extend_size = PAGE_SIZE * (n / PAGE_SIZE) + PAGE_SIZE;
        heap_end = sys_brk(heap_end + extend_size);
    }

    if (heap_end <= heap_start) {
        return NULL;
    }

    void *address = (void *)heap_index;
    heap_index += n;

    return address;
}

EXPORTABLE void *memcpy(
    void *restrict dest, const void *restrict src, size_t n
) {
    for (size_t i = 0; i < n; i++) {
        ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
    }

    return dest;
}

EXPORTABLE void signal() {
    fprintf(stderr, "signal unimplemented\n");
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
    int32_t file_no = _fileno(stream);
    fprintf_internal(file_no, format, arg);
    fprintf(stderr, "\nDEBUG: inserted newline manually\n");
    return 0;
}

EXPORTABLE void ___lc_codepage_func() {
    fprintf(stderr, "___lc_codepage_func unimplemented\n");
    exit(42);
}

EXPORTABLE void ___mb_cur_max_func() {
    fprintf(stderr, "___mb_cur_max_func unimplemented\n");
    exit(42);
}

EXPORTABLE int32_t *_errno() {
    return &errno;
}

EXPORTABLE void _lock([[maybe_unused]] int32_t locknum) {
}

EXPORTABLE void _unlock([[maybe_unused]] int32_t locknum) {
}

EXPORTABLE int32_t fputc(int32_t c, FILE *stream) {
    char c_char = (char)c;
    int32_t file_no = _fileno(stream);
    if (!print_len(file_no, &c_char, 1)) {
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

EXPORTABLE void *memset(void *s_buffer, int32_t c_value, size_t n_count) {
    for (size_t i = 0; i < n_count; i++) {
        ((uint8_t *)s_buffer)[i] = (uint8_t)c_value;
    }
    return s_buffer;
}

EXPORTABLE void strerror() {
    fprintf(stderr, "strerror unimplemented\n");
    exit(42);
}

EXPORTABLE void wcslen() {
    fprintf(stderr, "wcslen unimplemented\n");
    exit(42);
}
