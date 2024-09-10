#include "../../../loader/loader_lib.h"
#include "../../../loader/memory_map.h"
#include "../../../tiny_c/tiny_c.h"

ARM32_START_FUNCTION

#define assert(expr)                                                           \
    if (!(expr)) {                                                             \
        tiny_c_fprintf(STDERR, "%s:%x\n", __FILE__, __LINE__);                 \
        tiny_c_fprintf(STDERR, "%s\n", __func__);                              \
        tiny_c_fprintf(STDERR, "%s\n", #expr);                                 \
        tiny_c_exit(-1);                                                       \
    }

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
        &memory_regions_len,
        0
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
        &memory_regions_len,
        0
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

void loader_malloc_arena_align_test(void) {
    const int POINTER_SIZE = sizeof(size_t);

    size_t malloc_one = (size_t)loader_malloc_arena(15);
    size_t malloc_two = (size_t)loader_malloc_arena(15);

    assert(malloc_one % POINTER_SIZE == 0);
    assert(malloc_two % POINTER_SIZE == 0);
}

/* Tests divmod which can be called indriectly via modulo operator */
void aeabi_uidivmod_test(
    size_t numerator,
    size_t denominator,
    size_t expected_quotient,
    size_t expected_remainder
) {
    size_t quotient = divmod(numerator, denominator);
    size_t remainder = numerator % denominator;

    assert(quotient == expected_quotient);
    assert(remainder == expected_remainder);
}

void find_relocation_test(void) {
}

int main(void) {
    get_memory_regions_basic_test();
    get_memory_regions_offset_test();
    loader_malloc_arena_align_test();
    aeabi_uidivmod_test(13, 2, 6, 1);
    aeabi_uidivmod_test(12, 2, 6, 0);
    find_relocation_test();
}
