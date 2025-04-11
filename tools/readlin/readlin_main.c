#include "../../src/dlls/msvcrt.h"
#include "../../src/loader/linux/elf_tools.h"
#include <stddef.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        EXIT("Usage: readlin <file>\n");
    }

    char *filename = argv[1];
    int32_t fd = open(filename, O_RDONLY);
    if (fd < 0) {
        EXIT("file not found\n");
    }

    struct ElfData inferior_elf;
    if (!get_elf_data(fd, &inferior_elf)) {
        EXIT("error parsing elf data\n");
    }
    close(fd);

    /* General information */

    char *magic = (char *)inferior_elf.header.e_ident;
    char *type = inferior_elf.header.e_type == 1 ? "REL"
        : inferior_elf.header.e_type == 2        ? "EXEC"
        : inferior_elf.header.e_type == 3        ? "DYN"
                                                 : "UNKNOWN";
    const char *pie_display = inferior_elf.is_pie ? " - PIE" : "";

    printf("File: %s\n\n", filename);
    printf("PE Header:\n");
    printf("Magic: \\x%x%c%c%c\n", magic[0], magic[1], magic[2], magic[3]);
    printf("Class: %zd\n", inferior_elf.word_size);
    printf("Type: %d - %s%s\n", inferior_elf.header.e_type, type, pie_display);
    printf("Flags: %d\n", inferior_elf.header.e_flags);
    printf("Entry: 0x%zx\n", inferior_elf.header.e_entry);
    printf("Program headers start: 0x%zx\n", inferior_elf.header.e_phoff);
    printf("Section headers start: 0x%zx\n", inferior_elf.header.e_shoff);
    printf("Section header size: %d\n", inferior_elf.header.e_shentsize);
    printf("Section headers length: %d\n", inferior_elf.header.e_shnum);

    if (!inferior_elf.dynamic_data) {
        return 0;
    }

    /* Relocations */

    size_t var_relocations_len = inferior_elf.dynamic_data->var_relocations_len;
    printf("\nVariable Relocations (%zd):\n", var_relocations_len);
    for (size_t i = 0; i < var_relocations_len; i++) {
        struct Relocation *relocation =
            &inferior_elf.dynamic_data->var_relocations[i];
        printf(
            "0x%zx, %s, 0x%zx, 0x%zx, %s, '%s'\n",
            relocation->offset,
            relocation->type_name,
            relocation->addend,
            relocation->symbol.value,
            relocation->symbol.type_name,
            relocation->symbol.name
        );
    }

    size_t func_relocations_len =
        inferior_elf.dynamic_data->func_relocations_len;
    printf("\nFunction Relocations (%zd):\n", func_relocations_len);
    for (size_t i = 0; i < func_relocations_len; i++) {
        struct Relocation *relocation =
            &inferior_elf.dynamic_data->func_relocations[i];
        printf(
            "0x%zx, %s, 0x%zx, 0x%zx, %s, '%s'\n",
            relocation->offset,
            relocation->type_name,
            relocation->addend,
            relocation->symbol.value,
            relocation->symbol.type_name,
            relocation->symbol.name
        );
    }

    /* Global Offset Table */

    printf("\nGOT Entries(%zd):\n", inferior_elf.dynamic_data->got_entries_len);
    for (size_t i = 0; i < inferior_elf.dynamic_data->got_entries_len; i++) {
        struct GotEntry *got_entry = &inferior_elf.dynamic_data->got_entries[i];
        printf("%zd: 0x%zx, 0x%zx\n", i, got_entry->index, got_entry->value);
    }
}
