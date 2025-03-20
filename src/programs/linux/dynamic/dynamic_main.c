#include "../../../tiny_c/tiny_c.h"
#include "stddef.h"
#include <string.h>

extern int test_number_bss;
extern int test_number_data;

int get_test_number_data_internal_ref(void);

void set_test_number_data_internal_ref(int32_t val);

size_t add_many(size_t, size_t, size_t, size_t, size_t, size_t, size_t, size_t);

int main(void) {
    /* Call dynamic function leaf */
    size_t num1 = tiny_c_pow(2, 4);
    size_t num2 = tiny_c_pow(2, 4);

    /* Call dynamic function tree */
    tinyc_fputs("1st call\n", STDOUT);
    tinyc_fputs("2nd call\n", STDOUT);
    tiny_c_printf("%d + %d = %d\n", num1, num2, num1 + num2);

    /* 2nd shared lib dynamic leaf */
    const char *str = "how now brown cow";
    tiny_c_printf("2nd shared lib length of '%s': %d\n", str, strlen(str));

    /* Dynamic variable relocation */
    int32_t *dynamic_var_data = &test_number_data;
    tiny_c_printf("dynamic_var_data: %d\n", test_number_data);
    *dynamic_var_data = 54321;
    tiny_c_printf("dynamic_var_data: %d\n", *dynamic_var_data);
    int32_t *dynamic_var_bss = &test_number_bss;
    tiny_c_printf("dynamic_var_bss: %d\n", *dynamic_var_bss);
    *dynamic_var_bss = 54321;
    tiny_c_printf("dynamic_var_bss: %d\n", *dynamic_var_bss);
    tiny_c_printf(
        "get_test_number_data_internal_ref: %d\n",
        get_test_number_data_internal_ref()
    );
    set_test_number_data_internal_ref(54321);
    tiny_c_printf(
        "get_test_number_data_internal_ref: %d\n",
        get_test_number_data_internal_ref()
    );

    /* Dynamic variable relocation from shared object */
    char *os_release_buffer = malloc(0x1000);
    tiny_c_printf("malloc: %s\n", os_release_buffer == NULL ? "err" : "ok");

    /** Many parameter function */
    size_t add_many_result = add_many(1, 2, 3, 4, 5, 6, 7, 8);
    tiny_c_printf("add_many_result: %d\n", add_many_result);
}
