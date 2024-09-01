#include "../../../tiny_c/tiny_c.h"

ARM32_START_FUNCTION

int main(void) {
    // tiny_c_print_len(STDOUT, "1st call\n", 9);
    // tiny_c_print_len(STDOUT, "2nd call\n", 9);
    int32_t num = get_number();
    return num;
}
