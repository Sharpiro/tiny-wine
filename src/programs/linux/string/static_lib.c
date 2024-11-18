#include <stdint.h>

int32_t test_number_bss = 0;
int32_t test_number_data = 12345;

int get_test_number_data() {
    return test_number_data;
}
