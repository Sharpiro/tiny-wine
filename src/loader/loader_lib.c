#include "loader_lib.h"
#include "../tiny_c/tiny_c.h"
#include "log.h"
#include "memory_map.h"
#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>

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
        BAIL("runtime_relocation was null\n");
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

bool find_runtime_symbol(
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
        BAIL("got_entry was null\n");
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

bool log_memory_regions(void) {
    char *maps_buffer;
    if (!read_to_string("/proc/self/maps", &maps_buffer, 0x1000)) {
        BAIL("read failed\n");
    }

    LOGINFO("Mapped address regions:\n%s\n", maps_buffer);
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

bool find_symbols(
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
        } else if (elf_got_entry->is_library_virtual_base_address) {
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
bool get_memory_regions(
    const PROGRAM_HEADER *program_headers,
    size_t program_headers_len,
    MemoryRegionList *memory_regions
) {
    if (program_headers == NULL) {
        BAIL("program_headers cannot be null\n");
    }
    if (memory_regions == NULL) {
        BAIL("memory_regions cannot be null\n");
    }

    for (size_t i = 0; i < program_headers_len; i++) {
        const PROGRAM_HEADER *program_header = &program_headers[i];
        if (program_header->p_type != PT_LOAD) {
            continue;
        }
        if (program_header->p_filesz == 0 && program_header->p_offset != 0) {
            LOGWARNING(
                "PH %d: zero filesize w/ non-zero offset may not be "
                "unsupported\n",
                i + 1
            );
        }
        if (program_header->p_memsz != program_header->p_filesz) {
            LOGINFO("PH filesize != memsize\n", i + 1);
        }

        size_t file_offset = program_header->p_offset /
            program_header->p_align * program_header->p_align;
        size_t start = program_header->p_vaddr / program_header->p_align *
            program_header->p_align;
        size_t end = start +
            program_header->p_memsz / program_header->p_align *
                program_header->p_align +
            program_header->p_align;
        size_t max_region_address =
            program_header->p_vaddr + program_header->p_memsz;
        if (max_region_address > end) {
            LOGTRACE("Memory region %x extended due to offset\n", start);
            end += 0x1000;
        }

        struct MemoryRegion memory_region = (struct MemoryRegion){
            .start = start,
            .end = end,
            .is_direct_file_map = program_header->p_filesz > 0,
            .file_offset = file_offset,
            .permissions = program_header->p_flags,
        };
        MemoryRegionList_add(memory_regions, memory_region);
    }

    return true;
}
