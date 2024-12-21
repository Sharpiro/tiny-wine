#include <stddef.h>
#include <stdint.h>

#define SYS_write 0x01
#define SYS_exit 0x3c

struct SysArgs {
    size_t param_one;
    size_t param_two;
    size_t param_three;
    size_t param_four;
    size_t param_five;
    size_t param_six;
    size_t param_seven;
};

size_t write(int32_t file_handle, const char *data, size_t size);
