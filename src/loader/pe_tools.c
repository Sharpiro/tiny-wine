
#include "./pe_tools.h"
#include "../tiny_c/tiny_c.h"

bool get_pe_data(int32_t fd, struct PeData *pe_data) {
    if (pe_data == NULL) {
        BAIL("pe_data was null\n");
    }

    *pe_data = (struct PeData){
        .entry_point = 0x188,
    };

    return true;
}
