#include "../../../tiny_c/tiny_c.h"

ARM32_START_FUNCTION

size_t get42(void);

int main(void) {
    /* Call dynamic function leaf */
    size_t num1 = tiny_c_pow(2, 4);
    size_t num2 = tiny_c_pow(2, 4);

    /* Call dynamic function tree */
    tiny_c_print_len(STDOUT, "1st call\n", 9);
    tiny_c_print_len(STDOUT, "2nd call\n", 9);

    tiny_c_printf("%x + %x = %x\n", num1, num2, num1 + num2);

    /* Manipulate dynamic variable */
    int32_t *dynamic_var = &tinyc_errno;
    *dynamic_var = 42;
    tiny_c_printf("dynamic_var: %x\n", *dynamic_var);

    /* 42 */
    size_t var42 = get42();
    tiny_c_printf("shared lib: %x\n", var42);
}
