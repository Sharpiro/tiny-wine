#include "../../../loader/loader_lib.h"
#include "../../../loader/memory_map.h"
#include "../../../tiny_c/tiny_c.h"

#define assert(expr)                                                           \
    if (!(expr)) {                                                             \
        tiny_c_fprintf(STDERR, "%s:%x\n", __FILE__, __LINE__);                 \
        tiny_c_fprintf(STDERR, "%s\n", __func__);                              \
        tiny_c_fprintf(STDERR, "%s\n", #expr);                                 \
        tiny_c_exit(-1);                                                       \
    }

static void get_memory_regions_basic_test(void) {
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

    struct MemoryRegionsInfo memory_regions_info;
    bool result = get_memory_regions_info(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        0,
        &memory_regions_info
    );

    assert(result);
    assert(memory_regions_info.start == 0x10000);
    assert(memory_regions_info.end == 0x13000);
    assert(memory_regions_info.memory_regions_len == 2);
    assert(memory_regions_info.memory_regions[0].start == 0x10000);
    assert(memory_regions_info.memory_regions[0].end == 0x12000);
    assert(memory_regions_info.memory_regions[0].file_offset == 0);
    assert(memory_regions_info.memory_regions[1].start == 0x12000);
    assert(memory_regions_info.memory_regions[1].end == 0x13000);
    assert(memory_regions_info.memory_regions[1].file_offset == 0);
}

static void get_memory_regions_offset_test(void) {
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

    struct MemoryRegionsInfo memory_regions_info;
    bool result = get_memory_regions_info(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        0,
        &memory_regions_info
    );

    assert(result);
    assert(memory_regions_info.start == 0x10000);
    assert(memory_regions_info.end == 0x13000);
    assert(memory_regions_info.memory_regions_len == 2);
    assert(memory_regions_info.memory_regions[0].start == 0x10000);
    assert(memory_regions_info.memory_regions[0].end == 0x11000);
    assert(memory_regions_info.memory_regions[0].file_offset == 0);
    assert(memory_regions_info.memory_regions[1].start == 0x11000);
    assert(memory_regions_info.memory_regions[1].end == 0x13000);
    assert(memory_regions_info.memory_regions[1].file_offset == 0);
}

static void get_memory_regions_big_align_test(void) {
    PROGRAM_HEADER program_headers[] = {
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0,
            .p_vaddr = 0x10000,
            .p_memsz = 0x004e4,
            .p_flags = PF_R | PF_X,
            .p_align = 0x10000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0x000f58,
            .p_vaddr = 0x00020f58,
            .p_memsz = 0x000d0,
            .p_flags = PF_R | PF_W,
            .p_align = 0x10000,
        },
    };

    struct MemoryRegionsInfo memory_regions_info;
    bool result = get_memory_regions_info(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        0,
        &memory_regions_info
    );

    assert(result);
    assert(memory_regions_info.start == 0x10000);
    assert(memory_regions_info.end == 0x30000);
    assert(memory_regions_info.memory_regions_len == 2);
    assert(memory_regions_info.memory_regions[0].start == 0x10000);
    assert(memory_regions_info.memory_regions[0].end == 0x20000);
    assert(memory_regions_info.memory_regions[0].file_offset == 0);
    assert(memory_regions_info.memory_regions[1].start == 0x20000);
    assert(memory_regions_info.memory_regions[1].end == 0x30000);
    assert(memory_regions_info.memory_regions[1].file_offset == 0);
}

static void loader_malloc_arena_align_test(void) {
    const int POINTER_SIZE = sizeof(size_t);

    size_t malloc_one = (size_t)loader_malloc_arena(15);
    size_t malloc_two = (size_t)loader_malloc_arena(15);

    assert(malloc_one % POINTER_SIZE == 0);
    assert(malloc_two % POINTER_SIZE == 0);
}

/* Tests divmod which can be called indriectly via modulo operator */
static void aeabi_uidivmod_test(
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

static void get_runtime_function_local_lib_test(void) {
    struct RuntimeSymbol runtime_symbols[] = {
        {.value = 0x9d8, .name = "tiny_c_pow"},
    };

    size_t runtime_address;
    bool result = get_runtime_address(
        "tiny_c_pow",
        runtime_symbols,
        sizeof(runtime_symbols) / sizeof(struct RuntimeSymbol),
        &runtime_address
    );

    assert(result);
    assert(runtime_address == 0x9d8);
}

static void get_runtime_function_shared_lib_test(void) {
    struct RuntimeSymbol runtime_symbols[] = {
        {
            .value = 0,
            .name = "tiny_c_pow",
        },
        {
            .value = 0x209d8,
            .name = "tiny_c_pow",
        },
    };

    size_t runtime_address;
    bool result = get_runtime_address(
        "tiny_c_pow",
        runtime_symbols,
        sizeof(runtime_symbols) / sizeof(struct RuntimeSymbol),
        &runtime_address
    );

    assert(result);
    assert(runtime_address == 0x209d8);
}

int main(void) {
    get_memory_regions_basic_test();
    get_memory_regions_offset_test();
    get_memory_regions_big_align_test();
    loader_malloc_arena_align_test();
    aeabi_uidivmod_test(13, 2, 6, 1);
    aeabi_uidivmod_test(12, 2, 6, 0);
    get_runtime_function_local_lib_test();
    get_runtime_function_shared_lib_test();
}
