#include "msvcrt.h"
#include <stdarg.h>
#include <stdbool.h>

// typedef __builtin_va_list __gnuc_va_list;
// typedef __gnuc_va_list va_list;
// #define va_start(ap, param) __builtin_va_start(ap, param)
// #define va_end(ap) __builtin_va_end(ap)
// #define va_arg(ap, type) __builtin_va_arg(ap, type)

void DllMainCRTStartup(void) {
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wincompatible-library-redeclaration"

size_t strlen(const char *data) {
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

#pragma clang diagnostic pop

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

int32_t puts(const char *data) {
    print(STDOUT, data);
    print(STDOUT, "\n");
    return 0;
}

double pow(double x, double y) {
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
                // @todo: out of bounds
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

int32_t printf(const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(STDOUT, format, var_args);
    va_end(var_args);
    return 0;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-noreturn"

void exit(int32_t exit_code) {
    NtTerminateProcess((HANDLE)-1, exit_code);
}

#pragma clang diagnostic pop

size_t add_many_msvcrt(
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
