#include <dlls/twlibc.h>
#include <loader/linux/loader_lib.h>
#include <loader/windows/win_loader_lib.h>
#include <log.h>

#define tw_assert(expr)                                                        \
    if (!(expr)) {                                                             \
        fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);                        \
        fprintf(stderr, "%s\n", __func__);                                     \
        fprintf(stderr, "%s\n", #expr);                                        \
        exit(-1);                                                              \
    }

static void get_memory_regions_basic_test(void) {
    PROGRAM_HEADER program_headers[] = {
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0,
            .p_vaddr = 0x10000,
            .p_filesz = 0x15bc,
            .p_memsz = 0x15bc,
            .p_flags = PF_R | PF_X,
            .p_align = 0x1000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0x5bc,
            .p_vaddr = 0x125bc,
            .p_filesz = 0x10,
            .p_memsz = 0x10,
            .p_flags = PF_R | PF_W,
            .p_align = 0x1000,
        },
    };

    MemoryRegionList memory_regions = {
        .allocator = loader_malloc_arena,
    };
    bool result = get_memory_regions(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        &memory_regions
    );

    tw_assert(result);
    tw_assert(memory_regions.length == 2);
    tw_assert(memory_regions.data[0].start == 0x10000);
    tw_assert(memory_regions.data[0].end == 0x12000);
    tw_assert(memory_regions.data[0].file_offset == 0);
    tw_assert(memory_regions.data[1].start == 0x12000);
    tw_assert(memory_regions.data[1].end == 0x13000);
    tw_assert(memory_regions.data[1].file_offset == 0);
}

static void get_memory_regions_offset_test(void) {
    PROGRAM_HEADER program_headers[] = {
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0,
            .p_vaddr = 0x10000,
            .p_filesz = 0x23c,
            .p_memsz = 0x23c,
            .p_flags = PF_R | PF_X,
            .p_align = 0x1000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0xf78,
            .p_vaddr = 0x11f78,
            .p_filesz = 0x98,
            .p_memsz = 0x98,
            .p_flags = PF_R | PF_W,
            .p_align = 0x1000,
        },
    };

    MemoryRegionList memory_regions = {
        .allocator = loader_malloc_arena,
    };
    bool result = get_memory_regions(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        &memory_regions
    );

    tw_assert(result);
    tw_assert(memory_regions.length == 2);
    tw_assert(memory_regions.data[0].start == 0x10000);
    tw_assert(memory_regions.data[0].end == 0x11000);
    tw_assert(memory_regions.data[0].file_offset == 0);
    tw_assert(memory_regions.data[1].start == 0x11000);
    tw_assert(memory_regions.data[1].end == 0x13000);
    tw_assert(memory_regions.data[1].file_offset == 0);
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

    MemoryRegionList memory_regions = {
        .allocator = loader_malloc_arena,
    };
    bool result = get_memory_regions(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        &memory_regions
    );

    tw_assert(result);
    tw_assert(memory_regions.length == 2);
    tw_assert(memory_regions.data[0].start == 0x10000);
    tw_assert(memory_regions.data[0].end == 0x20000);
    tw_assert(memory_regions.data[0].file_offset == 0);
    tw_assert(memory_regions.data[1].start == 0x20000);
    tw_assert(memory_regions.data[1].end == 0x30000);
    tw_assert(memory_regions.data[1].file_offset == 0);
}

static void get_memory_regions_x86_test(void) {
    PROGRAM_HEADER program_headers[] = {
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0,
            .p_vaddr = 0x400000,
            .p_filesz = 0x1ec,
            .p_memsz = 0x1ec,
            .p_flags = PF_R,
            .p_align = 0x1000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0x1000,
            .p_vaddr = 0x401000,
            .p_filesz = 0x1400,
            .p_memsz = 0x1400,
            .p_flags = PF_R | PF_X,
            .p_align = 0x1000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0x3000,
            .p_vaddr = 0x403000,
            .p_filesz = 0x710,
            .p_memsz = 0x710,
            .p_flags = PF_R,
            .p_align = 0x1000,
        },
        (PROGRAM_HEADER){
            .p_type = PT_LOAD,
            .p_offset = 0,
            .p_vaddr = 0x404000,
            .p_filesz = 0,
            .p_memsz = 0x28,
            .p_flags = PF_R | PF_W,
            .p_align = 0x1000,
        },
    };

    MemoryRegionList memory_regions = {
        .allocator = loader_malloc_arena,
    };
    bool result = get_memory_regions(
        program_headers,
        sizeof(program_headers) / sizeof(PROGRAM_HEADER),
        &memory_regions
    );

    tw_assert(result);
    tw_assert(memory_regions.length == 4);
    tw_assert(memory_regions.data[0].start == 0x400000);
    tw_assert(memory_regions.data[0].end == 0x401000);
    tw_assert(memory_regions.data[0].is_direct_file_map == true);
    tw_assert(memory_regions.data[0].file_offset == 0);
    tw_assert(memory_regions.data[0].permissions == 4);
    tw_assert(memory_regions.data[1].start == 0x401000);
    tw_assert(memory_regions.data[1].end == 0x403000);
    tw_assert(memory_regions.data[1].is_direct_file_map == true);
    tw_assert(memory_regions.data[1].file_offset == 0x1000);
    tw_assert(memory_regions.data[1].permissions == 5);
    tw_assert(memory_regions.data[2].start == 0x403000);
    tw_assert(memory_regions.data[2].end == 0x404000);
    tw_assert(memory_regions.data[2].is_direct_file_map == true);
    tw_assert(memory_regions.data[2].file_offset == 0x3000);
    tw_assert(memory_regions.data[2].permissions == 4);
    tw_assert(memory_regions.data[3].start == 0x404000);
    tw_assert(memory_regions.data[3].end == 0x405000);
    tw_assert(memory_regions.data[3].is_direct_file_map == false);
    tw_assert(memory_regions.data[3].file_offset == 0);
    tw_assert(memory_regions.data[3].permissions == 6);
}

static void get_memory_regions_win_test(void) {
    struct WinSectionHeader program_headers[] = {
        (struct WinSectionHeader){
            .name = ".text",
            .virtual_size = 0x48,
            .virtual_base_address = 0x1000,
            .file_offset = 0x400,
            .characteristics = 0x60000020,
        },
        (struct WinSectionHeader){
            .name = ".pdata",
            .virtual_size = 0x0c,
            .virtual_base_address = 0x2000,
            .file_offset = 0x600,
            .characteristics = 0x40000040,
        }
    };

    MemoryRegionList memory_regions = (MemoryRegionList){
        .allocator = loader_malloc_arena,
    };
    bool result = get_memory_regions_win(
        program_headers,
        sizeof(program_headers) / sizeof(struct WinSectionHeader),
        0x140000000,
        &memory_regions
    );

    tw_assert(result);
    tw_assert(memory_regions.length == 2);
    tw_assert(memory_regions.data[0].start == 0x140001000);
    tw_assert(memory_regions.data[0].end == 0x140002000);
    tw_assert(memory_regions.data[0].is_direct_file_map == false);
    tw_assert(memory_regions.data[0].file_offset == 0x400);
    tw_assert(memory_regions.data[0].file_size == 0x48);
    tw_assert(memory_regions.data[0].permissions == 5);
    tw_assert(memory_regions.data[1].start == 0x140002000);
    tw_assert(memory_regions.data[1].end == 0x140003000);
    tw_assert(memory_regions.data[1].is_direct_file_map == false);
    tw_assert(memory_regions.data[1].file_offset == 0x600);
    tw_assert(memory_regions.data[1].file_size == 0x0c);
    tw_assert(memory_regions.data[1].permissions == 4);
}

static void loader_malloc_arena_align_test(void) {
    const int POINTER_SIZE = sizeof(size_t);

    size_t malloc_one = (size_t)loader_malloc_arena(15);
    size_t malloc_two = (size_t)loader_malloc_arena(15);

    tw_assert(malloc_one % POINTER_SIZE == 0);
    tw_assert(malloc_two % POINTER_SIZE == 0);
}

static void get_runtime_function_local_lib_test(void) {
    struct RuntimeSymbol runtime_symbols[] = {
        {.value = 0x9d8, .name = "pow"},
    };

    const struct RuntimeSymbol *runtime_symbol;
    bool result = find_runtime_symbol(
        "pow",
        runtime_symbols,
        sizeof(runtime_symbols) / sizeof(struct RuntimeSymbol),
        0,
        &runtime_symbol
    );

    tw_assert(result);
    tw_assert(runtime_symbol->value == 0x9d8);
}

static void get_runtime_function_shared_lib_test(void) {
    struct RuntimeSymbol runtime_symbols[] = {
        {
            .value = 0,
            .name = "pow",
        },
        {
            .value = 0x209d8,
            .name = "pow",
        },
    };

    const struct RuntimeSymbol *runtime_symbol;
    bool result = find_runtime_symbol(
        "pow",
        runtime_symbols,
        sizeof(runtime_symbols) / sizeof(struct RuntimeSymbol),
        0,
        &runtime_symbol
    );

    tw_assert(result);
    tw_assert(runtime_symbol->value == 0x209d8);
}

int main(void) {
    LOGINFO("get_memory_regions_basic_test\n");
    get_memory_regions_basic_test();
    LOGINFO("get_memory_regions_offset_test\n");
    get_memory_regions_offset_test();
    LOGINFO("get_memory_regions_big_align_test\n");
    get_memory_regions_big_align_test();
    LOGINFO("get_memory_regions_x86_test\n");
    get_memory_regions_x86_test();
    LOGINFO("get_memory_regions_win_test\n");
    get_memory_regions_win_test();
    LOGINFO("loader_malloc_arena_align_test\n");
    loader_malloc_arena_align_test();
    LOGINFO("get_runtime_function_local_lib_test\n");
    get_runtime_function_local_lib_test();
    LOGINFO("get_runtime_function_shared_lib_test\n");
    get_runtime_function_shared_lib_test();
}
