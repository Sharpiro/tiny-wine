#include "../../../dlls/msvcrt.h"
#include "../../../dlls/tinyc_stdio.h"
#include <stdlib.h>

int start_inferior() {
    int32_t num1 = (int32_t)pow(2, 4);
    int32_t num2 = (int32_t)pow(2, 4);
    printf("%d + %d = %d\n", num1, num2, num1 + num2);

    return 0;
}

void mainCRTStartup() {
    int result = start_inferior();
    exit(result);
}

void _start() {
    int result = start_inferior();
    exit(result);
}
