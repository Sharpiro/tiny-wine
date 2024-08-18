#include "../../../tiny_c/tiny_c.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

// const char *CONST_STRING = "const string";
// const int CONST_ZERO = 0;
// const int CONST_ONE = 1;

// int data_int = 1;
// char *data_string = "string";

// int bss_int = 0;
// char *bss_string = NULL;

ARM32_START_FUNCTION

int main(void) {
    // tiny_c_printf("inline string\n");

    // tiny_c_printf("CONST_STRING %s\n", CONST_STRING);
    // tiny_c_printf("CONST_ZERO %x\n", CONST_ZERO);
    // tiny_c_printf("CONST_ONE %x\n", CONST_ONE);

    // tiny_c_printf("data_int %x\n", data_int);
    // tiny_c_printf("data_string %s\n", data_string);

    // tiny_c_printf("bss_int %x\n", bss_int);
    // tiny_c_printf("bss_string %x\n", bss_string);

    char *malloc_string = tinyc_malloc_arena(0x1000);
    if (malloc_string == NULL) {
        tiny_c_fprintf(STDERR, "malloc failed\n");
        tiny_c_exit(-1);
    }
    tiny_c_printf("malloc_string* %x\n", malloc_string);
    // malloc_string = tinyc_malloc_arena(0x1000);
    // tiny_c_printf("malloc_string %x\n", malloc_string);
    memcpy(malloc_string, "abcz", 5);
    tiny_c_printf("malloc_string %s\n", malloc_string);

    tiny_c_exit(0);
}
