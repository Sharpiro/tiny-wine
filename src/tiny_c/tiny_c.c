#include "tiny_c.h"
#include "tinyc_sys.h"
#include <asm-generic/errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>

int32_t tinyc_errno = 0;

// @todo: 0 initial value doesn't work
size_t tinyc_heap_start = 0;
size_t tinyc_heap_end = 0;
size_t tinyc_heap_index = 0;

#ifdef AMD64

#define MMAP SYS_mmap

size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    __asm__("mov rdi, %0" : : "r"(sys_args->param_one));
    __asm__("mov rsi, %0" : : "r"(sys_args->param_two));
    __asm__("mov rdx, %0" : : "r"(sys_args->param_three));
    __asm__("mov rcx, %0" : : "r"(sys_args->param_four));
    __asm__("mov r8, %0" : : "r"(sys_args->param_five));
    __asm__("mov r9, %0" : : "r"(sys_args->param_six));
    __asm__("mov r10, %0" : : "r"(sys_args->param_seven));
    __asm__("mov rax, %0" : : "r"(sys_no));
    __asm__("syscall" : "=r"(result) : :);

    return result;
}

#endif

#ifdef ARM32

#define MMAP SYS_mmap2
// #define MMAP 0xc0

// @todo: don't know how to clobber 7 registers
size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    __asm__("mov r0, %[p1]\n"
            "mov r1, %[p2]\n"
            "mov r2, %[p3]\n"
            "mov r3, %[p4]\n"
            "mov r4, %[p5]\n"
            "mov r5, %[p6]\n"
            "mov r6, %[p7]\n"
            "mov r7, %[sysno]\n"
            "svc #0\n"
            "mov %[res], r0\n"
            : [res] "=r"(result)
            : [p1] "r"(sys_args->param_one),
              [p2] "r"(sys_args->param_two),
              [p3] "r"(sys_args->param_three),
              [p4] "r"(sys_args->param_four),
              [p5] "r"(sys_args->param_five),
              [p6] "r"(sys_args->param_six),
              [p7] "r"(sys_args->param_seven),
              [sysno] "r"(sys_no)
            : "r0", "r1", "r2", "r3", "r4");

    return result;
}

#endif

void tiny_c_print_len(int32_t file_handle, const char *data, size_t size) {
    struct SysArgs args = {
        .param_one = (size_t)file_handle,
        .param_two = (size_t)data,
        .param_three = size,
    };
    tiny_c_syscall(SYS_write, &args);
}

void tiny_c_print(int32_t file_handle, const char *data) {
    if (data == NULL) {
        tiny_c_print_len(file_handle, "<null>", 6);
        return;
    }

    size_t size = 0;
    for (size_t i = 0; true; i++) {
        if (data[i] == 0x00) {
            size = i;
            break;
        }
    }

    tiny_c_print_len(file_handle, data, size);
}

static void tiny_c_newline(int32_t file_handle) {
    struct SysArgs args = (struct SysArgs){
        .param_one = (size_t)file_handle,
        .param_two = (size_t)"\n",
        .param_three = 1,
    };
    tiny_c_syscall(SYS_write, &args);
}

void tiny_c_puts(int32_t file_handle, const char *data) {
    tiny_c_print(file_handle, data);
    tiny_c_newline(file_handle);
}

size_t tiny_c_pow(size_t x, size_t y) {
    size_t product = 1;
    for (size_t i = 0; i < y; i++) {
        product *= x;
    }

    return product;
}

void tiny_c_print_number_hex(int32_t file_handle, size_t num) {
    const size_t MAX_DIGITS = sizeof(num) * 2;
    const char *HEX_CHARS = "0123456789abcdef";

    char num_buffer[32] = {0};
    num_buffer[0] = '0';
    num_buffer[1] = 'x';

    size_t current_base = tiny_c_pow(0x10, MAX_DIGITS - 1);
    size_t buffer_index = 2;
    for (size_t i = 0; i < MAX_DIGITS; i++, buffer_index++) {
        size_t digit = num / current_base;
        num_buffer[buffer_index] = HEX_CHARS[digit];
        size_t digit_value = digit * current_base;
        num -= digit_value;
        current_base >>= 4;
    }

    struct SysArgs args = {
        .param_one = (size_t)file_handle,
        .param_two = (size_t)num_buffer,
        .param_three = buffer_index,
    };
    tiny_c_syscall(SYS_write, &args);
}

struct PrintItem {
    size_t start;
    size_t length;
    char formatter;
};

static void tiny_c_fprintf_internal(
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
        case 's': {
            char *data = va_arg(var_args, char *);
            tiny_c_print(file_handle, data);
            break;
        }
        case 'x': {
            size_t data = va_arg(var_args, size_t);
            tiny_c_print_number_hex(file_handle, data);
            break;
        }
        case 0x00: {
            const char *data = format + print_item.start;
            tiny_c_print_len(file_handle, data, print_item.length);
            break;
        }
        default: {
            tiny_c_print(file_handle, "<unknown>");
            break;
        }
        }
    }

    va_end(var_args);
}

void tiny_c_printf(const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    tiny_c_fprintf_internal(STDOUT, format, var_args);
    va_end(var_args);
}

void tiny_c_fprintf(int32_t file_handle, const char *format, ...) {
    va_list var_args;
    va_start(var_args, format);
    tiny_c_fprintf_internal(file_handle, format, var_args);
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
    struct SysArgs args = {
        .param_one = address,
        .param_two = length,
        .param_three = prot,
        .param_four = flags,
        .param_five = (size_t)fd,
        .param_six = offset / 0x1000,
    };
    size_t result = tiny_c_syscall(MMAP, &args);
    int32_t err = (int32_t)result;
    if (err < 1) {
        tinyc_errno = -err;
        return MAP_FAILED;
    }

    return (void *)result;
}

// @todo: x64 convention disrespect could be a register clobber bug

// void *tiny_c_mmap(size_t address, size_t length, size_t prot, size_t
// flags,
//                   size_t fd, size_t offset) {
//     struct SysArgs args = {
//         .param_one = address,
//         .param_two = length,
//         .param_three = prot,
//         .param_seven = flags, // @note: disrespects x64 calling
//         convention .param_five = fd, .param_six = offset,
//     };
//     void *result = (void *)tiny_c_syscall(MMAP, &args);

//     return result;
// }

size_t tiny_c_munmap(size_t address, size_t length) {
    struct SysArgs args = {
        .param_one = address,
        .param_two = length,
    };
    size_t result = tiny_c_syscall(SYS_munmap, &args);

    return result;
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

int tiny_c_strcmp(const void *buffer_a, const void *buffer_b) {
    for (size_t i = 0; true; i++) {
        u_int8_t a = ((u_int8_t *)buffer_a)[i];
        u_int8_t b = ((u_int8_t *)buffer_b)[i];
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
    case EACCES:
        return "Permission denied";
    case EINVAL:
        return "Invalid argument";
    case ERANGE:
        return "Math result not representable";
    default:
        return "Unknown";
    }
}

// @todo: configurable logging
// @todo: alignment
void *tinyc_malloc_arena(size_t n) {
    const size_t PAGE_SIZE = 0x1000;

    if (tinyc_heap_start == 0) {
        // tiny_c_printf("requested start %x\n", 0);
        tinyc_heap_start = tinyc_sys_brk(0);
        // tiny_c_printf("actual start %x\n", tinyc_heap_start);
        tinyc_heap_end = tinyc_heap_start;
        tinyc_heap_index = tinyc_heap_start;
    }
    if (tinyc_heap_index + n > tinyc_heap_end) {
        size_t extend_size = PAGE_SIZE * (n / PAGE_SIZE) + PAGE_SIZE;
        // tiny_c_printf("requested end %x\n", tinyc_heap_end + extend_size);
        tinyc_heap_end = tinyc_sys_brk(tinyc_heap_end + extend_size);
        // tiny_c_printf("actual end %x\n", tinyc_heap_end);
        if (tinyc_heap_end <= tinyc_heap_start) {
            return NULL;
        }
    }

    void *address = (void *)tinyc_heap_index;
    tinyc_heap_index += n;

    return address;
}

void tinyc_free_arena(void) {
    tinyc_heap_index = tinyc_heap_start;
}

off_t tinyc_lseek(int fd, off_t offset, int whence) {
    return tinyc_sys_lseek((size_t)fd, offset, (size_t)whence);
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

    return NULL;
}

#endif

#ifdef AMD64

struct stat tiny_c_stat(const char *path) {
    struct stat file_stat;
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)&file_stat,
    };
    tiny_c_syscall(SYS_stat, &args);

    return file_stat;
}

#endif

void print_buffer(uint8_t *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (i > 0 && i % 2 == 0) {
            tiny_c_printf("\n", buffer[i]);
        }
        tiny_c_printf("%x, ", buffer[i]);
    }
    tiny_c_printf("\n");
}
