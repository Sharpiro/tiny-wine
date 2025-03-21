#include "msvcrt.h"
#include "./macros.h"
#include "ntdll.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef VERBOSE

#define LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__);

#else

#define LOG(fmt, ...)                                                          \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

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

EXPORTABLE char *_acmdln = NULL;

char *command_line_array[0x100] = {};

void DllMainCRTStartup(void) {
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

static void fputs(const char *data, int32_t file_handle) {
    if (data == NULL) {
        const char NULL_STRING[] = "(null)";
        print_len(file_handle, NULL_STRING, sizeof(NULL_STRING) - 1);
        return;
    }

    size_t str_len = strlen(data);
    print_len(file_handle, data, str_len);
}

EXPORTABLE int32_t puts(const char *data) {
    fputs(data, STDOUT);
    fputs("\n", STDOUT);
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
    bool is_large_formatter;
};

static void fprintf_internal(
    int32_t file_handle, const char *format, va_list var_args
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

EXPORTABLE void *__iob_func(void) {
    return WIN_FILE_INTERNAL_LIST;
}

EXPORTABLE int32_t _fileno(FILE *stream) {
    _WinFileInternal *internal_file = (_WinFileInternal *)stream;
    return internal_file->fileno_lazy_maybe;
}

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

EXPORTABLE void __initenv() {
    fprintf(stderr, "__initenv unimplemented\n");
    exit(42);
}

/**
 * Initialize locale-specific information.
 */
EXPORTABLE int __lconv_init() {
    return 0;
}

/**
 * Informs runtime if app is console or gui.
 */
EXPORTABLE void __set_app_type([[maybe_unused]] int type) {
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

static char *to_char_pointer(wchar_t *data) {
    size_t length = wcslen(data);
    char *buffer = malloc(length + 1);
    size_t i;
    for (i = 0; i < length; i++) {
        buffer[i] = (char)data[i];
    }
    buffer[i] = 0x00;
    return buffer;
}

typedef void (*_PVFV)(void);

/**
 * Initializes app with an array of functions
 */
EXPORTABLE void _initterm(_PVFV *first, _PVFV *last) {
    TEB *teb = NULL;
    __asm__("mov %0, gs:[0x30]\n" : "=r"(teb));
    UNICODE_STRING *command_line =
        &teb->ProcessEnvironmentBlock->ProcessParameters->CommandLine;
    _acmdln = to_char_pointer((wchar_t *)command_line->Buffer);
    LOG("_initterm\n");
    LOG("_acmdln: %p, %s \n", _acmdln, _acmdln);

    size_t len = (size_t)(last - first);
    for (size_t i = 0; i < len; i++) {
        _PVFV func = first[i];
        if (func == NULL) {
            continue;
        }
        LOG("_initterm func %zd\n", i);
        func();
    }
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

/**
 * @param do_wild_card If nonzero, enables wildcard expansion for
 command-line arguments (only relevant in certain CRT implementations).
 * @param start_info Reserved for interanl use.
 */
EXPORTABLE int __getmainargs(
    int *pargc,
    char ***pargv,
    char ***penvp,
    [[maybe_unused]] int do_wild_card,
    [[maybe_unused]] void *start_info
) {
    LOG("__getmainargs\n");

    char *current_cmd = _acmdln;

    int arg_count = 0;
    while (true) {
        char *start = current_cmd;
        current_cmd = strchrnul(current_cmd, ' ');
        size_t str_len = (size_t)current_cmd - (size_t)start;
        char *cmd_part = malloc(str_len + 1);
        memcpy(cmd_part, start, str_len);
        cmd_part[str_len] = 0x00;
        command_line_array[arg_count] = cmd_part;
        arg_count += 1;

        LOG("current_word: '%s'\n", cmd_part);

        if (*current_cmd == 0x00) {
            break;
        }
        current_cmd += 1;
    }

    *pargc = arg_count;
    *pargv = command_line_array;
    *penvp = NULL;
    return 0;
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
    LOG("inserted newline manually\n");
    return 0;
}

/* Returns the code page identifier for the current locale */
EXPORTABLE int ___lc_codepage_func() {
    fprintf(stderr, "___lc_codepage_func unimplemented\n");
    exit(42);
    // const int CODE_PAGE_WINDOWS = 1252;
    // const int CODE_PAGE_UTF8 = 65001;
    // return CODE_PAGE_WINDOWS;
}

/* Returns the maximum number of bytes in a multibyte character for the
 * current locale */
EXPORTABLE int ___mb_cur_max_func() {
    fprintf(stderr, "___mb_cur_max_func unimplemented\n");
    exit(42);
    // return 1;
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

EXPORTABLE void strerror() {
    fprintf(stderr, "strerror unimplemented\n");
    exit(42);
}
