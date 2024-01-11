#include <elf.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define STDOUT 1
#define NO_LIBC 1

struct SysArgs {
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
};

uint64_t tiny_c_syscall(uint64_t sys_no, struct SysArgs *sys_args) {
    uint64_t result = 0;

    asm("mov rdi, %0" : : "r"(sys_args->rdi));
    asm("mov rsi, %0" : : "r"(sys_args->rsi));
    asm("mov rdx, %0" : : "r"(sys_args->rdx));
    asm("mov rcx, %0" : : "r"(sys_args->rcx));
    asm("mov r8, %0" : : "r"(sys_args->r8));
    asm("mov r9, %0" : : "r"(sys_args->r9));
    asm("mov r10, %0" : : "r"(sys_args->r10));
    asm("mov rax, %0" : : "r"(sys_no));
    asm("syscall" : "=r"(result) : :);

    return result;
}

void tiny_c_print_len(const char *data, size_t size) {
    struct SysArgs args = {
        .rdi = STDOUT,
        .rsi = (uint64_t)data,
        .rdx = size,
    };
    tiny_c_syscall(SYS_write, &args);
}

void tiny_c_print(const char *data) {
    int size = 0;
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
        .rdi = STDOUT,
        .rsi = (uint64_t) "\n",
        .rdx = 1,
    };
    tiny_c_syscall(SYS_write, &args);
}

void tiny_c_puts(const char *data) {
    tiny_c_print(data);
    tiny_c_newline();
}

uint64_t tiny_c_pow(uint64_t x, uint64_t y) {
    size_t product = 1;
    for (size_t i = 0; i < y; i++) {
        product *= x;
    }

    return product;
}

void tiny_c_print_number(uint64_t num) {
    const char *HEX_CHARS = "0123456789abcdef";
    const size_t MAX_DIGITS = 16;

    char num_buffer[32] = {0};
    num_buffer[0] = '0';
    num_buffer[1] = 'x';

    uint64_t current_base = tiny_c_pow(0x10, MAX_DIGITS - 1);
    for (size_t i = 0; i < MAX_DIGITS; i++) {
        uint64_t digit = num / current_base;
        size_t j = i + 2;
        num_buffer[j] = HEX_CHARS[digit];
        uint64_t digit_value = digit * current_base;
        num -= digit_value;
        current_base /= 0x10;
    }

    struct SysArgs args = {
        .rdi = STDOUT,
        .rsi = (uint64_t)num_buffer,
        .rdx = 18,
    };
    tiny_c_syscall(SYS_write, &args);
}

struct PrintItem {
    uint64_t start;
    uint64_t length;
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
            uint64_t data = va_arg(var_args, uint64_t);
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

void tiny_c_exit(int code) {
    struct SysArgs args = {.rdi = code};
    tiny_c_syscall(SYS_exit, &args);
}

struct stat tiny_c_stat(const char *path) {
    struct stat file_stat;
    struct SysArgs args = {
        .rdi = (uint64_t)path,
        .rsi = (uint64_t)&file_stat,
    };
    tiny_c_syscall(SYS_stat, &args);

    return file_stat;
}

uint64_t tiny_c_fopen(const char *path) {
    struct SysArgs args = {
        .rdi = (uint64_t)path,
        .rsi = O_RDWR,
    };
    uint64_t fd = tiny_c_syscall(SYS_open, &args);

    return fd;
}

void tiny_c_fclose(uint64_t fd) {
    struct SysArgs args = {
        .rdi = fd,
    };
    tiny_c_syscall(SYS_close, &args);
}

void *tiny_c_mmap(size_t address, size_t length, size_t prot, size_t flags,
                  uint64_t fd, size_t offset) {
    struct SysArgs args = {
        .rdi = address,
        .rsi = length,
        .rdx = prot,
        .r10 = flags, // @note: disrespects calling convention
        .r8 = fd,
        .r9 = offset,
    };
    void *result = (void *)tiny_c_syscall(SYS_mmap, &args);

    return result;
}

size_t tiny_c_munmap(size_t address, size_t length) {
    struct SysArgs args = {
        .rdi = address,
        .rsi = length,
    };
    size_t result = tiny_c_syscall(SYS_munmap, &args);

    return result;
}
