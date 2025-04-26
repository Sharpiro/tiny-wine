#include <dlls/twlibc.h>
#include <stddef.h>
#include <stdint.h>
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

int main(void) {
    printf("inline string\n");

    printf("CONST_STRING %s\n", CONST_STRING);
    printf("CONST_ZERO %d\n", CONST_ZERO);
    printf("CONST_ONE %d\n", CONST_ONE);

    printf("data_int %d\n", data_int);
    printf("data_string %s\n", data_string);

    printf("bss_int %d\n", bss_int);
    printf("bss_string %s\n", bss_string);

    char *malloc_string = malloc(0x1000);
    memcpy(malloc_string, "abcd", 5);
    printf("malloc_string %s\n", malloc_string);

    printf("lib test_number_data: %d\n", test_number_data);
    test_number_data = 54321;
    printf("lib test_number_data: %d\n", test_number_data);
    printf("lib test_number_bss: %d\n", test_number_bss);
    test_number_bss = 1;
    printf("lib test_number_bss: %d\n", test_number_bss);
    printf("lib get_test_number_data: %d\n", get_test_number_data());
}
