#include "../../../dlls/msvcrt.h"
#include "./win_dynamic_lib.h"

int main() {
    printf("oh boy\n");
    exit(42);
    // @todo: return doesn't work
    // return 42;
}
