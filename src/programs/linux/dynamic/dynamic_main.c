#include "../../../tiny_c/tiny_c.h"
#include "string.h"

extern int test_number_bss;
extern int test_number_data;

int get_test_number_data_internal_ref(void);

int main(void) {
    /* Call dynamic function leaf */
    size_t num1 = tiny_c_pow(2, 4);
    size_t num2 = tiny_c_pow(2, 4);

    /* Call dynamic function tree */
    tiny_c_print_len(STDOUT, "1st call\n", 9);
    tiny_c_print_len(STDOUT, "2nd call\n", 9);

    tiny_c_printf("%d + %d = %d\n", num1, num2, num1 + num2);

    /* Manipulate dynamic variable */
    int32_t *dynamic_var = &tinyc_errno;
    tiny_c_printf("dynamic_var: %d\n", *dynamic_var);
    *dynamic_var = 42;
    tiny_c_printf("dynamic_var: %d\n", *dynamic_var);

    /* 2nd shared lib */
    const char *str = "how now brown cow";
    tiny_c_printf("2nd shared lib length of '%s': %x\n", str, strlen(str));

    /* Dynamic variable relocation */
    tiny_c_printf("lib test_number_data: %x\n", test_number_data);
    test_number_data = 0x54321;
    tiny_c_printf("lib test_number_data: %x\n", test_number_data);
    tiny_c_printf("lib test_number_bss: %x\n", test_number_bss);
    test_number_bss = 0x54321;
    tiny_c_printf("lib test_number_bss: %x\n", test_number_bss);
    tiny_c_printf(
        "lib get_test_number_data_internal_ref: %x\n",
        get_test_number_data_internal_ref()
    );
}
