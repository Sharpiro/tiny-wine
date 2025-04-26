#include "../../../dlls/twlibc.h"
#include "stddef.h"
#include <string.h>

extern int test_number_bss;
extern int test_number_data;

int get_test_number_data_internal_ref(void);

void set_test_number_data_internal_ref(int32_t val);

size_t add_many(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);

int main(void) {
    /* Call dynamic function leaf */
    size_t num1 = (size_t)pow(2, 4);
    size_t num2 = (size_t)pow(2, 4);

    /* Call dynamic function tree */
    fputs("1st call\n", stdout);
    fputs("2nd call\n", stdout);
    printf("%zd + %zd = %zd\n", num1, num2, num1 + num2);

    /* 2nd shared lib dynamic leaf */
    const char *str = "how now brown cow";
    printf("2nd shared lib length of '%s': %zd\n", str, strlen(str));

    /* Dynamic variable relocation */
    int32_t *dynamic_var_data = &test_number_data;
    printf("dynamic_var_data: %d\n", test_number_data);
    *dynamic_var_data = 54321;
    printf("dynamic_var_data: %d\n", *dynamic_var_data);
    int32_t *dynamic_var_bss = &test_number_bss;
    printf("dynamic_var_bss: %d\n", *dynamic_var_bss);
    *dynamic_var_bss = 54321;
    printf("dynamic_var_bss: %d\n", *dynamic_var_bss);
    printf(
        "get_test_number_data_internal_ref: %d\n",
        get_test_number_data_internal_ref()
    );
    set_test_number_data_internal_ref(54321);
    printf(
        "get_test_number_data_internal_ref: %d\n",
        get_test_number_data_internal_ref()
    );

    /* Dynamic variable relocation from shared object */
    char *os_release_buffer = malloc(0x1000);
    printf("malloc: %s\n", os_release_buffer == NULL ? "err" : "ok");

    /** Many parameter function */
    size_t add_many_result = add_many(1, 2, 3, 4, 5, 6, 7, 8);
    printf("add_many_result: %zd\n", add_many_result);
}
