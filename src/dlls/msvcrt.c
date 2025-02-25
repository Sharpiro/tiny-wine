#include "msvcrt.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"
#pragma clang diagnostic ignored "-Winvalid-noreturn"
#pragma clang diagnostic ignored "-Wdll-attribute-on-redeclaration"
#pragma clang diagnostic ignored "-Wbuiltin-requires-header"

/**
 * Variables must be marked as exportable with -nostdlib.
 * 'Some' functions require it as well.
 * If you use it on one function in a file, you must use it on all.
 */
#if defined(_WIN32)
#define EXPORTABLE __declspec(dllexport)
#else
#define EXPORTABLE
#endif

static size_t heap_start = 0;
static size_t heap_end = 0;
static size_t heap_index = 0;
static int32_t errno = 0;

void DllMainCRTStartup(void) {
}

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

    NtWriteFile(
        win_handle,
        NULL,
        NULL,
        NULL,
        NULL,
        (PVOID)data,
        (ULONG)length,
        NULL,
        NULL
    );

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
    num_buffer[0] = '0';
    num_buffer[1] = 'x';

    size_t current_base = (size_t)pow(0x10, MAX_DIGITS - 1);
    size_t buffer_index = 2;
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
    // exit(5);
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

EXPORTABLE void __C_specific_handler() {
}

EXPORTABLE void __getmainargs() {
}

EXPORTABLE void __initenv() {
}

typedef struct WinFileInternal {
    uint32_t a;
    uint32_t b;
    uint32_t x;
    uint32_t c;
    uint32_t fileno32;
    uint32_t e;
    uint32_t fileno64;
} WinFileInternal;

WinFileInternal TEMP[] = {
    {.fileno32 = 0, .fileno64 = 0},
    {.fileno32 = 2, .fileno64 = 2}, //@todo: stderr
    {.fileno32 = 1, .fileno64 = 1},
    {.fileno32 = 3, .fileno64 = 3}, //@todo: stderr
    {.fileno32 = 4, .fileno64 = 4}, //@todo: stderr
};

EXPORTABLE void *__iob_func(void) {
    return (void *)TEMP;
}

EXPORTABLE void __lconv_init() {
}

EXPORTABLE void __set_app_type() {
}

EXPORTABLE void __setusermatherr() {
}

EXPORTABLE void _acmdln() {
}

EXPORTABLE void _amsg_exit() {
}

EXPORTABLE void _cexit() {
}

EXPORTABLE void _commode() {
}

EXPORTABLE void _fmode() {
}

EXPORTABLE void _initterm() {
}

EXPORTABLE void _onexit() {
}

EXPORTABLE void abort() {
}

EXPORTABLE void calloc() {
}

EXPORTABLE int fprintf(
    uint8_t *__restrict __stream, const char *__restrict __format, ...
) {
    exit(4);
}

EXPORTABLE void free() {
}

EXPORTABLE size_t fwrite(
    const void *__restrict __ptr,
    size_t __size,
    size_t __n,
    uint8_t *__restrict __s
) {
    exit(3);
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
    exit(109);
}

EXPORTABLE void strncmp() {
    exit(108);
}

EXPORTABLE int vfprintf(
    uint8_t *__restrict __s,
    const char *__restrict __format,
    __gnuc_va_list __arg
) {
    exit(2);
}

EXPORTABLE void ___lc_codepage_func() {
    exit(107);
}

EXPORTABLE void ___mb_cur_max_func() {
    exit(106);
}

EXPORTABLE int32_t *_errno() {
    return &errno;
}

EXPORTABLE void _lock([[maybe_unused]] int32_t locknum) {
    // exit(101);
}

EXPORTABLE void _unlock([[maybe_unused]] int32_t locknum) {
    // exit(102);
}

// EXPORTABLE void fputc() {
// @todo: stderr
// EXPORTABLE int fputc(int c, ssize_t stream) {
int x = 0;
EXPORTABLE int fputc(int c, WinFileInternal *file) {
    // 0x70
    // fileno
    // uint64_t x = file[0x08]; // 1, 0
    // uint32_t x = file[0x0d];
    // printf("%x: %d\n", file, file[16]);
    // exit(file->fileno64);
    // for (size_t i = 0; i < 0x160; i++) {
    //     // print_number_decimal(1, file[i]);
    //     // print_len(1, "\n", 1);
    //     uint64_t val = file[i];
    //     if (val > 0 && val < 5) {
    //         printf("%d: %x\n", i, val);
    //     }
    // }
    // if (stream != -11) {
    //     exit(11);
    // }
    char c_char = (char)c;
    // print_len((int32_t)stream, &c_char, 1);
    // print_len(1, &c_char, 1);
    print_number_decimal(1, file->fileno64);
    print_len(1, "\n", 1);
    return (int)(unsigned char)c_char;
}

EXPORTABLE void localeconv() {
    exit(103);
}

EXPORTABLE void *memset(void *s_buffer, int c_value, size_t n_count) {
    for (size_t i = 0; i < n_count; i++) {
        ((uint8_t *)s_buffer)[i] = (uint8_t)c_value;
    }
    return s_buffer;
}

EXPORTABLE void strerror() {
    exit(104);
}

EXPORTABLE void wcslen() {
    exit(105);
}
