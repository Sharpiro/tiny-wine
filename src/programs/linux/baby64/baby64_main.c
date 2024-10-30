#include "../../../tiny_c/tiny_c.h"
#include <unistd.h>

int main(int argc, char **argv) {
    tiny_c_printf("argc: %x\n", argc);
    tiny_c_printf("argv[0]: %s\n", argv[0]);
}
