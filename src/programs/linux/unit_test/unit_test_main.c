#include "../../../loader/memory_map.h"
// #include "../../../tiny_c/tiny_c.h"
#include <stdio.h>

int main(void) {
    if (!get_memory_regions(NULL, NULL, NULL)) {
        return -1;
    }
}
