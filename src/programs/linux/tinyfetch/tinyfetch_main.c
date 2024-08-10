#include "../../../tiny_c/tiny_c.h"
#include "../../../tiny_c/tinyc_sys.h"

ARM32_START_FUNCTION

int main(void) {
    // int32_t pid = tiny_c_get_pid();
    // tiny_c_printf("pid: %x\n", pid);

    // char cwd_buffer[100];
    // const char *cwd = tiny_c_get_cwd(cwd_buffer, 100);
    // tiny_c_printf("cwd: '%s'\n", cwd);

    // void *brk_addr = NULL;
    // if (tinyc_brk(brk_addr)) {
    //     tiny_c_fprintf(
    //         STDERR,
    //         "brk failed, %x, %s\n",
    //         tinyc_errno,
    //         tinyc_strerror(tinyc_errno)
    //     );
    //     return -1;
    // }

    // uint8_t *heap_start = (void *)tinyc_sys_brk(0);
    // uint8_t *heap_end = (void *)tinyc_sys_brk((size_t)heap_start + 0x1000);
    // tiny_c_printf("heap_start: '%x'\n", heap_start);
    // tiny_c_printf("heap_end: '%x'\n", heap_end);
    // tiny_c_printf("heap_len: '%x'\n", heap_end - heap_start);
    uint8_t *buffer = tinyc_malloc(500);
    if (buffer == NULL) {
        tiny_c_fprintf(STDERR, "malloc failed\n", buffer);
        return -1;
    }
    tiny_c_printf("p: %x\n", buffer);
    tiny_c_printf("n: %x\n", *buffer);
}
