#include "../../src/loader//pe_tools.h"
#include "../../src/tiny_c/tiny_c.h"
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        EXIT("file required\n");
    }

    char *filename = argv[1];
    int32_t fd = tiny_c_open(filename, O_RDONLY);
    if (fd < 0) {
        EXIT("file not found\n");
    }

    struct PeData pe_data;
    if (!get_pe_data(fd, &pe_data)) {
        EXIT("failed getting PE data\n");
    }
    tiny_c_close(fd);

    tiny_c_printf("%s\n", filename);
    tiny_c_printf("Entry point address: %x\n", pe_data.entrypoint);
    tiny_c_printf("Section headers len: %d\n", pe_data.section_headers_len);
}
