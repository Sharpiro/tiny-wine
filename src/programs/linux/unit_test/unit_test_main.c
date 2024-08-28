#include "../../../loader/memory_map.h"
#include <assert.h>

// #define PANIC(fmt, ...)                                                        \
//     fprintf(stderr, fmt, ##__VA_ARGS__);                                       \
//     exit(-1);

void get_memory_regions_basic_test(void) {
    PROGRAM_HEADER program_headers[] = {
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0,
            .p_vaddr = 0x10000,
            .p_memsz = 0x015bc,
            .p_flags = PF_R | PF_X,
            .p_align = 0x1000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0x0005bc,
            .p_vaddr = 0x000125bc,
            .p_memsz = 0x00010,
            .p_flags = PF_R | PF_W,
            .p_align = 0x1000,
        },
    };

    struct MemoryRegion *memory_regions;
    size_t memory_regions_len;
    bool result = get_memory_regions(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        &memory_regions,
        &memory_regions_len
    );

    assert(result);
    assert(memory_regions_len == 2);
    assert(memory_regions[0].start == 0x10000);
    assert(memory_regions[0].end == 0x12000);
    assert(memory_regions[0].file_offset == 0);
    assert(memory_regions[1].start == 0x12000);
    assert(memory_regions[1].end == 0x13000);
    assert(memory_regions[1].file_offset == 0);
}

void get_memory_regions_offset_test(void) {
    PROGRAM_HEADER program_headers[] = {
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0,
            .p_vaddr = 0x10000,
            .p_memsz = 0x0023c,
            .p_flags = PF_R | PF_X,
            .p_align = 0x1000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0x000f78,
            .p_vaddr = 0x00011f78,
            .p_memsz = 0x00098,
            .p_flags = PF_R | PF_W,
            .p_align = 0x1000,
        },
    };

    struct MemoryRegion *memory_regions;
    size_t memory_regions_len;
    bool result = get_memory_regions(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        &memory_regions,
        &memory_regions_len
    );

    assert(result);
    assert(memory_regions_len == 2);
    assert(memory_regions[0].start == 0x10000);
    assert(memory_regions[0].end == 0x11000);
    assert(memory_regions[0].file_offset == 0);
    assert(memory_regions[1].start == 0x11000);
    assert(memory_regions[1].end == 0x13000);
    assert(memory_regions[1].file_offset == 0);
}

int main(void) {
    get_memory_regions_basic_test();
    get_memory_regions_offset_test();
}
