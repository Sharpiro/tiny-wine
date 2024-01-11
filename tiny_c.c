#include <elf.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
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

void tiny_c_print_len(const char *data, size_t size) {
    struct SysArgs args = {
        .param_one = STDOUT,
        .param_two = (size_t)data,
        .param_three = size,
    };
    tiny_c_syscall(SYS_write, &args);
}

void tiny_c_print(const char *data) {
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
        .param_two = (size_t) "\n",
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
    const char *HEX_CHARS = "0123456789abcdef";
    const size_t MAX_DIGITS = 16;

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
        current_base /= 0x10;
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

void tiny_c_exit(size_t code) {
    struct SysArgs args = {.param_one = code};
    tiny_c_syscall(SYS_exit, &args);
}

struct stat tiny_c_stat(const char *path) {
    struct stat file_stat;
    struct SysArgs args = {
        .param_one = (size_t)path,
        .param_two = (size_t)&file_stat,
    };
    tiny_c_syscall(SYS_stat, &args);

    return file_stat;
}

size_t tiny_c_fopen(const char *path) {
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
        .param_seven = flags, // @note: disrespects calling convention
        .param_five = fd,
        .param_six = offset,
    };
    void *result = (void *)tiny_c_syscall(SYS_mmap, &args);

    return result;
}

size_t tiny_c_munmap(size_t address, size_t length) {
    struct SysArgs args = {
        .param_one = address,
        .param_two = length,
    };
    size_t result = tiny_c_syscall(SYS_munmap, &args);

    return result;
}
