#include "tiny_c.h"
#include "tinyc_sys.h"
#include <asm-generic/errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>

// @todo: naming convention tiny_c_xyz vs tinyc_xyz

int32_t tinyc_errno = 0;
size_t tinyc_heap_start = 0;
size_t tinyc_heap_end = 0;
size_t tinyc_heap_index = 0;

#ifdef AMD64

#define MMAP SYS_mmap

#endif

#ifdef ARM32

#define MMAP SYS_mmap2

#endif

static void tinyc_print_len(
    int32_t file_handle, const char *data, size_t length
) {
    struct SysArgs args = {
        .param_one = (size_t)file_handle,
        .param_two = (size_t)data,
        .param_three = length,
    };
    tiny_c_syscall(SYS_write, &args);
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

void tinyc_fputs(const char *data, int32_t file_handle) {
    if (data == NULL) {
        const char NULL_STRING[] = "(null)";
        tinyc_print_len(file_handle, NULL_STRING, sizeof(NULL_STRING) - 1);
        return;
    }

    size_t str_len = strlen(data);
    tinyc_print_len(file_handle, data, str_len);
}

size_t tiny_c_pow(size_t x, size_t y) {
    size_t product = 1;
    for (size_t i = 0; i < y; i++) {
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

    size_t current_base = tiny_c_pow(0x10, MAX_DIGITS - 1);
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

    struct SysArgs args = {
        .param_one = (size_t)file_handle,
        .param_two = (size_t)num_buffer,
        .param_three = buffer_index,
    };
    tiny_c_syscall(SYS_write, &args);
}

static void print_number_decimal(int32_t file_handle, size_t num) {
    const size_t MAX_DIGITS = sizeof(num) * 2;
    const char *NUMBER_CHARS = "0123456789";

    char num_buffer[32] = {0};
    size_t current_base = tiny_c_pow(10, MAX_DIGITS - 1);
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
    tiny_c_syscall(SYS_write, &args);
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
    struct PrintItem print_items[256] = {0};

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
            tinyc_print_len(file_handle, data, print_item.length);
            break;
        }
        case 's': {
            char *data = va_arg(var_args, char *);
            tinyc_fputs(data, file_handle);
            break;
        }
        case 'c': {
            char data = va_arg(var_args, char);
            tinyc_print_len(file_handle, &data, 1);
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
            tinyc_fputs("<unknown>", file_handle);
            break;
        }
        }
    }

    va_end(var_args);
}

void tiny_c_printf(const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(STDOUT, format, var_args);
    va_end(var_args);
}

void tiny_c_fprintf(int32_t file_handle, const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    fprintf_internal(file_handle, format, var_args);
    va_end(var_args);
}

void tiny_c_exit(int32_t code) {
    struct SysArgs args = {.param_one = (size_t)code};
    tiny_c_syscall(SYS_exit, &args);
}

int32_t tiny_c_open(const char *path, int flags) {
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)flags,
    };
    int32_t result = (int32_t)tiny_c_syscall(SYS_open, &args);
    int32_t err = (int32_t)result;
    if (err < 1) {
        tinyc_errno = -err;
        return -1;
    }

    return result;
}

void tiny_c_close(int32_t fd) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
    };
    tiny_c_syscall(SYS_close, &args);
}

ssize_t tiny_c_read(int32_t fd, void *buf, size_t count) {
    struct SysArgs args = {
        .param_one = (size_t)fd,
        .param_two = (size_t)buf,
        .param_three = count,
    };
    return (ssize_t)tiny_c_syscall(SYS_read, &args);
}

void *tiny_c_mmap(
    size_t address,
    size_t length,
    size_t prot,
    size_t flags,
    int32_t fd,
    size_t offset
) {
#ifdef ARM32
    struct SysArgs args = {
        .param_one = address,
        .param_two = length,
        .param_three = prot,
        .param_four = flags,
        .param_five = (size_t)fd,
        .param_six = offset / 0x1000,
    };
#elif defined AMD64
    struct SysArgs args = {
        .param_one = address,
        .param_two = length,
        .param_three = prot,
        .param_seven = flags, // @note: disrespects x64 calling convention
        .param_five = (size_t)fd,
        .param_six = offset,
    };
#endif

    size_t result = tiny_c_syscall(MMAP, &args);
    ssize_t err = (ssize_t)result;
    if (err < 1) {
        tinyc_errno = (int32_t)-err;
        return MAP_FAILED;
    }

    return (void *)result;
}

size_t tiny_c_munmap(size_t address, size_t length) {
    struct SysArgs args = {
        .param_one = address,
        .param_two = length,
    };
    size_t result = tiny_c_syscall(SYS_munmap, &args);

    return result;
}

int32_t tiny_c_mprotect(void *address, size_t length, int32_t protection) {
    struct SysArgs args = {
        .param_one = (size_t)address,
        .param_two = length,
        .param_three = (size_t)protection,
    };
    size_t result = tiny_c_syscall(SYS_mprotect, &args);
    int32_t err = (int32_t)result;
    if (err < 1) {
        tinyc_errno = -err;
    }

    return err;
}

int tiny_c_memcmp(const void *buffer_a, const void *buffer_b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        u_int8_t a = ((u_int8_t *)buffer_a)[i];
        u_int8_t b = ((u_int8_t *)buffer_b)[i];
        if (a != b) {
            return a - b;
        }
    }

    return 0;
}

int tiny_c_strcmp(const char *buffer_a, const char *buffer_b) {
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

int32_t tiny_c_get_pid(void) {
    struct SysArgs args = {0};
    return (int32_t)tiny_c_syscall(SYS_getpid, &args);
}

char *tiny_c_get_cwd(char *buffer, size_t size) {
    struct SysArgs args = {
        .param_one = (size_t)buffer,
        .param_two = size,
    };
    size_t result = tiny_c_syscall(SYS_getcwd, &args);
    if (result < 1) {
        return NULL;
    }

    return buffer;
}

const char *tinyc_strerror(int32_t err_number) {
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

    if (tinyc_heap_start == 0) {
        tinyc_heap_start = tinyc_sys_brk(0);
        tinyc_heap_end = tinyc_heap_start;
        tinyc_heap_index = tinyc_heap_start;
    }
    if (tinyc_heap_index + n > tinyc_heap_end) {
        size_t extend_size = PAGE_SIZE * (n / PAGE_SIZE) + PAGE_SIZE;
        tinyc_heap_end = tinyc_sys_brk(tinyc_heap_end + extend_size);
    }

    if (tinyc_heap_end <= tinyc_heap_start) {
        return NULL;
    }

    void *address = (void *)tinyc_heap_index;
    tinyc_heap_index += n;

    return address;
}

off_t tinyc_lseek(int32_t fd, off_t offset, int32_t whence) {
    return tinyc_sys_lseek((uint32_t)fd, offset, (uint32_t)whence);
}

int32_t tinyc_uname(struct utsname *uname) {
    int32_t result = (int32_t)tinyc_sys_uname(uname);
    if (result < 0) {
        tinyc_errno = -result;
        return -1;
    }

    return result;
}

uid_t getuid(void) {
    struct SysArgs args = {0};
    uid_t result = (uid_t)tiny_c_syscall(SYS_getuid, &args);
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

#ifdef ARM32

uint32_t __aeabi_uidiv(uint32_t numerator, uint32_t denominator) {
    if (denominator == 0) {
        return 0xffffffff;
    }
    if (denominator == numerator) {
        return 1;
    }
    if (denominator >= numerator) {
        return 0;
    }

    size_t denominator_increment = denominator;
    size_t count = 0;
    while (denominator_increment <= numerator &&
           denominator_increment >= denominator) {
        count++;
        denominator_increment += denominator;
    }

    return count;
}

inline uint32_t divmod(uint32_t numerator, uint32_t denominator) {
    uint32_t quotient = __aeabi_uidiv(numerator, denominator);
    uint32_t remainder = numerator - quotient * denominator;
    __asm__("mov r1, %0\n" : : "r"(remainder));
    return quotient;
}

uint32_t __aeabi_uidivmod(uint32_t numerator, uint32_t denominator) {
    return divmod(numerator, denominator);
}

#endif

#ifdef AMD64

struct stat stat(const char *path) {
    struct stat file_stat;
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)&file_stat,
    };
    tiny_c_syscall(SYS_stat, &args);

    return file_stat;
}

#endif
