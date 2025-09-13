#include <stdio.h>

#ifdef DYNAMIC
const char *LINK_TYPE = "dynamically linked";
#else
const char *LINK_TYPE = "statically linked";
#endif

#ifdef WINDOWS
const char *OS = "Windows";
#else
const char *OS = "Linux";
#endif

int main() {
    printf("Hello %s %s %s!\n", OS, LINK_TYPE, "Tiny Wine");
}
