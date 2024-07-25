#include "tiny_c.h"
#include <elf.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define STDOUT 1
#define NO_LIBC 1

struct SysArgs {
    size_t param_one;
    size_t param_two;
    size_t param_three;
    size_t param_four;
    size_t param_five;
    size_t param_six;
    size_t param_seven;
};

#ifdef AMD64

#define MMAP SYS_mmap

size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    asm("mov rdi, %0" : : "r"(sys_args->param_one));
    asm("mov rsi, %0" : : "r"(sys_args->param_two));
    asm("mov rdx, %0" : : "r"(sys_args->param_three));
    asm("mov rcx, %0" : : "r"(sys_args->param_four));
    asm("mov r8, %0" : : "r"(sys_args->param_five));
    asm("mov r9, %0" : : "r"(sys_args->param_six));
    asm("mov r10, %0" : : "r"(sys_args->param_seven));
    asm("mov rax, %0" : : "r"(sys_no));
    asm("syscall" : "=r"(result) : :);

    return result;
}

#endif

#ifdef ARM32

#define MMAP SYS_mmap2

// @todo: don't know how to clobber 7 registers
// @todo: asm vs asm volatile
size_t tiny_c_syscall(size_t sys_no, struct SysArgs *sys_args) {
    size_t result = 0;

    asm("mov r0, %[p1]\n"
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
        : [p1] "g"(sys_args->param_one), [p2] "g"(sys_args->param_two),
          [p3] "g"(sys_args->param_three), [p4] "g"(sys_args->param_four),
          [p5] "g"(sys_args->param_five), [p6] "g"(sys_args->param_six),
          [p7] "g"(sys_args->param_seven), [sysno] "r"(sys_no)
        : "r0", "r1", "r2", "r3", "r4");

    return result;
}

#endif

void tiny_c_print_len(const char *data, size_t size) {
    struct SysArgs args = {
        .param_one = STDOUT,
        .param_two = (size_t)data,
        .param_three = size,
    };
    tiny_c_syscall(SYS_write, &args);
}

void tiny_c_print(const char *data) {
    if (data == NULL) {
        tiny_c_print_len("<null>", 6);
        return;
    }

    size_t size = 0;
    for (size_t i = 0; true; i++) {
        if (data[i] == 0x00) {
            size = i;
            break;
        }
    }

    tiny_c_print_len(data, size);
}

void tiny_c_newline(void) {
    struct SysArgs args = (struct SysArgs){
        .param_one = STDOUT,
        .param_two = (size_t)"\n",
        .param_three = 1,
    };
    tiny_c_syscall(SYS_write, &args);
}

void tiny_c_puts(const char *data) {
    tiny_c_print(data);
    tiny_c_newline();
}

size_t tiny_c_pow(size_t x, size_t y) {
    size_t product = 1;
    for (size_t i = 0; i < y; i++) {
        product *= x;
    }

    return product;
}

void tiny_c_print_number(size_t num) {
    const size_t MAX_DIGITS = sizeof(num) * 2;
    const char *HEX_CHARS = "0123456789abcdef";

    char num_buffer[32] = {0};
    num_buffer[0] = '0';
    num_buffer[1] = 'x';

    size_t current_base = tiny_c_pow(0x10, MAX_DIGITS - 1);
    for (size_t i = 0; i < MAX_DIGITS; i++) {
        size_t digit = num / current_base;
        size_t j = i + 2;
        num_buffer[j] = HEX_CHARS[digit];
        size_t digit_value = digit * current_base;
        num -= digit_value;
        current_base >>= 4;
    }

    struct SysArgs args = {
        .param_one = STDOUT,
        .param_two = (size_t)num_buffer,
        .param_three = 18,
    };
    tiny_c_syscall(SYS_write, &args);
}

struct PrintItem {
    size_t start;
    size_t length;
    char formatter;
};

void tiny_c_printf(const char *format, ...) {
    size_t print_items_index = 0;
    size_t print_items_count = 0;
    struct PrintItem print_items[256] = {0};
    size_t var_args_count = 0;

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
            var_args_count++;
        }
    }

    struct PrintItem print_item = {
        .start = last_print_index,
        .length = i - last_print_index,
    };
    print_items[print_items_index++] = print_item;
    print_items_count++;

    va_list var_args;
    va_start(var_args, var_args_count);

    for (size_t i = 0; i < print_items_count; i++) {
        struct PrintItem print_item = print_items[i];
        switch (print_item.formatter) {
        case 's': {
            char *data = va_arg(var_args, char *);
            tiny_c_print(data);
            break;
        }
        case 'x': {
            size_t data = va_arg(var_args, size_t);
            tiny_c_print_number(data);
            break;
        }
        case 0x00: {
            const char *data = format + print_item.start;
            tiny_c_print_len(data, print_item.length);
            break;
        }
        default: {
            tiny_c_print("<unknown>");
            break;
        }
        }
    }

    va_end(var_args);
}

void tiny_c_exit(int32_t code) {
    struct SysArgs args = {.param_one = code};
    tiny_c_syscall(SYS_exit, &args);
}

// @todo: docs say err should return -1, but i'm seeing -2, maybe related to
//        ENOENT + no libc errno thread global
ssize_t tiny_c_open(const char *path) {
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = O_RDWR,
    };
    size_t fd = tiny_c_syscall(SYS_open, &args);

    return fd;
}

void tiny_c_fclose(size_t fd) {
    struct SysArgs args = {
        .param_one = fd,
    };
    tiny_c_syscall(SYS_close, &args);
}

void *tiny_c_mmap(size_t address, size_t length, size_t prot, size_t flags,
                  size_t fd, size_t offset) {
    struct SysArgs args = {
        .param_one = address,
        .param_two = length,
        .param_three = prot,
        .param_four = flags,
        .param_five = fd,
        .param_six = offset,
    };
    void *result = (void *)tiny_c_syscall(MMAP, &args);

    return result;
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

// NOLINTBEGIN(clang-diagnostic-incompatible-library-redeclaration)

void memset(char *s_buffer, char c_value, size_t n_count) {
    for (size_t i = 0; i < n_count; i++) {
        s_buffer[i] = c_value;
    }
}

// @todo: 'abort'?
void memcpy(void) {
    tiny_c_printf("unimplemented\n");
    tiny_c_exit(-1);
}

// NOLINTEND(clang-diagnostic-incompatible-library-redeclaration)

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
