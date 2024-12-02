
#include <stddef.h>
#include <stdint.h>

struct PeData {
    size_t entry_point;
};

bool get_pe_data(int32_t fd, struct PeData *elf_data);
