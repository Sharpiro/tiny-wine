#include "./win_dynamic_lib.h"
#include <stdio.h>

int main() {
    int x = 1;
    int y = 1;
    int result = lib_add(x, y);
    printf("%d + %d + %d = %d\n", lib_var_data, x, y, result);
}
