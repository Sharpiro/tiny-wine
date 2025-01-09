#include "../../../tiny_c/tiny_c.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

const char *CONST_STRING = "const string";
const int CONST_ZERO = 0;
const int CONST_ONE = 1;

int data_int = 1;
char *data_string = "string";

int bss_int = 0;
char *bss_string = NULL;

extern int test_number_bss;
extern int test_number_data;

int get_test_number_data();

// @todo:
extern size_t tinyc_heap_start;
extern size_t tinyc_heap_end;
extern size_t tinyc_heap_index;

int main(void) {
    tiny_c_printf("inline string\n");

    tiny_c_printf("CONST_STRING %s\n", CONST_STRING);
    tiny_c_printf("CONST_ZERO %d\n", CONST_ZERO);
    tiny_c_printf("CONST_ONE %d\n", CONST_ONE);

    tiny_c_printf("data_int %d\n", data_int);
    tiny_c_printf("data_string %s\n", data_string);

    tiny_c_printf("bss_int %d\n", bss_int);
    tiny_c_printf("bss_string %s\n", bss_string);

    tiny_c_fprintf(STDERR, "tinyc_heap_start %x\n", tinyc_heap_start);
    tiny_c_fprintf(STDERR, "tinyc_heap_end %x\n", tinyc_heap_end);
    tiny_c_fprintf(STDERR, "tinyc_heap_index %x\n", tinyc_heap_index);
    tiny_c_fprintf(STDERR, "mallocing...\n");
    char *malloc_string = tinyc_malloc_arena(0x1000);
    tiny_c_fprintf(STDERR, "tinyc_heap_start %x\n", tinyc_heap_start);
    tiny_c_fprintf(STDERR, "tinyc_heap_end %x\n", tinyc_heap_end);
    tiny_c_fprintf(STDERR, "tinyc_heap_index %x\n", tinyc_heap_index);
    if (malloc_string == NULL) {
        tiny_c_fprintf(STDERR, "malloc failed\n");
        tiny_c_exit(-1);
    }
    memcpy(malloc_string, "abcd", 5);
    tiny_c_printf("malloc_string %s\n", malloc_string);

    tiny_c_printf("lib test_number_data: %d\n", test_number_data);
    test_number_data = 54321;
    tiny_c_printf("lib test_number_data: %d\n", test_number_data);
    tiny_c_printf("lib test_number_bss: %d\n", test_number_bss);
    test_number_bss = 1;
    tiny_c_printf("lib test_number_bss: %d\n", test_number_bss);
    tiny_c_printf("lib get_test_number_data: %d\n", get_test_number_data());

    tiny_c_exit(0);
}
