#include "loader_lib.h"
#include "../tiny_c/tiny_c.h"
#include "elf_tools.h"
#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>

int32_t loader_log_handle = STDERR;

static uint8_t *loader_buffer = NULL;
static size_t loader_heap_index = 0;

void *loader_malloc_arena(size_t n) {
    if (loader_buffer == NULL) {
        loader_buffer = tiny_c_mmapx86(
            LOADER_BUFFER_ADDRESS,
            LOADER_BUFFER_LEN,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
            -1,
            0
        );
        if (loader_buffer == MAP_FAILED) {
            LOADER_LOG("map failed\n");
            return NULL;
        }
    }

    const size_t POINTER_SIZE = sizeof(size_t);
    size_t alignment_mod = n % POINTER_SIZE;
    size_t aligned_end =
        alignment_mod == 0 ? n : n + POINTER_SIZE - alignment_mod;
    if (loader_heap_index + aligned_end > LOADER_BUFFER_LEN) {
        LOADER_LOG("size exceeded\n");
        return NULL;
    }

    void *address = (void *)(loader_buffer + loader_heap_index);
    loader_heap_index += aligned_end;

    return address;
}

void loader_free_arena(void) {
    loader_heap_index = 0;
}

bool find_runtime_relocation(
    const struct RuntimeRelocation *runtime_relocations,
    size_t runtime_relocations_len,
    size_t relocation_offset,
    const struct RuntimeRelocation **runtime_relocation
) {
    if (runtime_relocations == NULL) {
        BAIL("runtime_relocations was null\n");
    }
    if (runtime_relocation == NULL) {
        BAIL("runtime_relocation was null");
    }

    for (size_t j = 0; j < runtime_relocations_len; j++) {
        const struct RuntimeRelocation *curr_relocation =
            &runtime_relocations[j];
        if (curr_relocation->offset == relocation_offset) {
            *runtime_relocation = curr_relocation;
            return true;
        }
    }

    return false;
}

bool get_runtime_symbol(
    const char *name,
    const struct RuntimeSymbol *runtime_symbols,
    size_t runtime_symbols_len,
    size_t ignore_val,
    const struct RuntimeSymbol **symbol
) {
    if (name == NULL) {
        BAIL("relocation_name was null\n");
    }
    if (runtime_symbols == NULL) {
        BAIL("runtime_symbols was null\n");
    }
    if (symbol == NULL) {
        BAIL("relocation_address was null\n");
    }

    for (size_t i = 0; i < runtime_symbols_len; i++) {
        const struct RuntimeSymbol *curr_runtime_symbol = &runtime_symbols[i];
        if (curr_runtime_symbol->value == 0) {
            continue;
        }
        if (curr_runtime_symbol->value == ignore_val) {
            continue;
        }
        if (tiny_c_strcmp(curr_runtime_symbol->name, name) == 0) {
            *symbol = curr_runtime_symbol;
            return true;
        }
    }

    return false;
}

bool find_got_entry(
    const struct RuntimeGotEntry *got_entries,
    size_t got_entries_len,
    size_t offset,
    struct RuntimeGotEntry **got_entry
) {
    if (got_entries == NULL) {
        BAIL("got_entries was null\n");
    }
    if (got_entry == NULL) {
        BAIL("got_entry was null");
    }

    for (size_t j = 0; j < got_entries_len; j++) {
        const struct RuntimeGotEntry *curr_got_entry = &got_entries[j];
        if (curr_got_entry->index == offset) {
            *got_entry = (struct RuntimeGotEntry *)curr_got_entry;
            return true;
        }
    }

    return false;
}

bool read_to_string(const char *path, char **content, size_t size) {
    char *buffer = loader_malloc_arena(size);
    if (buffer == NULL) {
        BAIL("loader_malloc_arena failed\n");
    }

    int32_t fd = tiny_c_open(path, O_RDONLY);
    tiny_c_read(fd, buffer, size);
    tiny_c_close(fd);
    *content = buffer;

    return true;
}

bool print_memory_regions(void) {
    char *maps_buffer;
    if (!read_to_string("/proc/self/maps", &maps_buffer, 0x1000)) {
        BAIL("read failed\n");
    }

    LOADER_LOG("Mapped address regions:\n%s\n", maps_buffer);
    return true;
}

bool get_function_relocations(
    const struct DynamicData *dyn_data,
    size_t dyn_offset,
    struct RuntimeRelocation **runtime_func_relocations,
    size_t *runtime_func_relocations_len
) {
    *runtime_func_relocations = loader_malloc_arena(
        sizeof(struct RuntimeRelocation) * dyn_data->func_relocations_len
    );

    for (size_t i = 0; i < dyn_data->func_relocations_len; i++) {
        struct Relocation *curr_relocation = &dyn_data->func_relocations[i];
        size_t value = curr_relocation->symbol.value == 0
            ? 0
            : dyn_offset + curr_relocation->symbol.value;
        struct RuntimeRelocation runtime_relocation = {
            .offset = dyn_offset + curr_relocation->offset,
            .value = value,
            .name = curr_relocation->symbol.name,
            .type = curr_relocation->type,
            .lib_dyn_offset = dyn_offset,
        };
        (*runtime_func_relocations)[i] = runtime_relocation;
    }

    *runtime_func_relocations_len = dyn_data->func_relocations_len;
    return true;
}

bool get_runtime_symbols(
    const struct DynamicData *dyn_data,
    size_t lib_dyn_offset,
    RuntimeSymbolList *runtime_symbols
) {
    for (size_t i = 0; i < dyn_data->symbols_len; i++) {
        struct Symbol *curr_symbol = &dyn_data->symbols[i];
        size_t value =
            curr_symbol->value == 0 ? 0 : lib_dyn_offset + curr_symbol->value;
        struct RuntimeSymbol runtime_symbol = {
            .value = value,
            .name = curr_symbol->name,
            .size = curr_symbol->size,
        };
        if (!RuntimeSymbolList_add(runtime_symbols, runtime_symbol)) {
            BAIL("RuntimeSymbolList_add failed\n");
        }
    }

    return true;
}

bool get_runtime_got(
    const struct DynamicData *dyn_data,
    size_t lib_dyn_offset,
    size_t dynamic_linker_callback_address,
    size_t *got_lib_dyn_offset_table,
    RuntimeGotEntryList *runtime_got_entries
) {
    for (size_t j = 0; j < dyn_data->got_entries_len; j++) {
        struct GotEntry *elf_got_entry = &dyn_data->got_entries[j];
        size_t runtime_value;
        size_t lib_dynamic_offset = 0;
        if (elf_got_entry->is_loader_callback) {
            runtime_value = (size_t)dynamic_linker_callback_address;
        } else if (elf_got_entry->is_library_base_address) {
            runtime_value = (size_t)got_lib_dyn_offset_table;
            lib_dynamic_offset = lib_dyn_offset;
        } else if (elf_got_entry->value == 0) {
            runtime_value = 0;
        } else {
            runtime_value = lib_dyn_offset + elf_got_entry->value;
        }

        struct RuntimeGotEntry runtime_got_entry = {
            .index = lib_dyn_offset + elf_got_entry->index,
            .value = runtime_value,
            .lib_dynamic_offset = lib_dynamic_offset,
        };
        RuntimeGotEntryList_add(runtime_got_entries, runtime_got_entry);
    }

    return true;
}
