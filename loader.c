#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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

void tiny_c_puts(const char *data) {
    tiny_c_print(data);

    struct SysArgs args = (struct SysArgs){
        .rdi = STDOUT,
        .rsi = (uint64_t) "\n",
        .rdx = 1,
    };
    tiny_c_syscall(SYS_write, &args);
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

    //  if formatter is not the end...
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
        if (print_item.formatter == 's') {
            char *data = va_arg(var_args, char *);
            tiny_c_print(data);
        } else if (print_item.formatter == 'd') {
            uint64_t data = va_arg(var_args, uint64_t);
            tiny_c_print_number(data);
        } else if (print_item.formatter == 0x00) {
            const char *data = format + print_item.start;
            tiny_c_print_len(data, print_item.length);
        } else {
            tiny_c_print("<unknown>");
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
    // void *mapped = (void *)syscall(SYS_mmap, NULL, size, PROT_READ,
    // MAP_PRIVATE, fd, 0);

    // struct SysArgs args = {
    //     .rdi = address,
    //     .rsi = length,
    //     .rdx = prot,
    //     .rcx = flags,
    //     .r8 = fd,
    //     .r9 = offset,
    // };
    struct SysArgs args = {
        .rdi = address,
        .rsi = length,
        .rdx = prot,
        .r10 = flags, // @note: disrespects calling convention
        .r8 = fd,
        .r9 = offset,
    };
    uint64_t temp_result = tiny_c_syscall(SYS_mmap, &args);
    void *result = (void *)temp_result;
    // void *result = (void *)tiny_c_syscall(SYS_mmap, &args);

    return result;
}

#if NO_LIBC
void _start(void) {
    const char *FILE_NAME = "test_program_c_linux/test.exe";

    // struct stat file_stat = tiny_c_stat(FILE_NAME);
    // tiny_c_printf("file: %s, size: %d\n", FILE_NAME, file_stat.st_size);

    uint64_t fd = tiny_c_fopen(FILE_NAME);
    tiny_c_printf("fd: %d\n", fd);

    // void *addr =
    //     tiny_c_mmap(0x500000, 0x20000, PROT_READ | PROT_WRITE | PROT_EXEC,
    //                 MAP_PRIVATE, fd, 0);
    void *addr =
        tiny_c_mmap(0x500000, 0x20000, PROT_READ | PROT_WRITE | PROT_EXEC,
                    MAP_PRIVATE | MAP_ANONYMOUS, fd, 0);
    if (addr == MAP_FAILED) {
        tiny_c_printf("map failed\n");
    }

    tiny_c_printf("map address: %d\n", (uint64_t)addr);
    tiny_c_printf("data: %s\n", addr);
    // tiny_c_fclose(fd);

    // void *program_map_buffer =
    //     mmap((void *)PROGRAM_ADDRESS_START, PROGRAM_SPACE_SIZE,
    //          PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE,
    //          file->_fileno, 0);

    tiny_c_exit(0);
}
#endif

#if !NO_LIBC
int main(void) {
    tiny_c_puts("test", 4);
    tiny_c_exit(0);
}
#endif
