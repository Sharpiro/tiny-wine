#include "../../../tiny_c/tiny_c.h"

ARM32_START_FUNCTION

int main(void) {
    /* Call dynamic function leaf */
    int32_t num = (int32_t)tiny_c_pow(2, 4);

    /* Call dynamic function tree */
    // tiny_c_print_len(STDOUT, "1st call\n", 9);

    return num;
}
