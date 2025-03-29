#include "../../src/loader/linux/elf_tools.h"
#include "../../src/tinyc/tinyc.h"
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
    char *type = inferior_elf.header.e_type == 2 ? "EXEC" : "UNKNOWN";

    printf("PE Header:\n");
    printf("File: %s\n", filename);
    printf("Magic: \\x%x%c%c%c\n", magic[0], magic[1], magic[2], magic[3]);
    printf("Class: %zd\n", inferior_elf.word_size);
    printf("Type: %s\n", type);
    printf("Entry: 0x%zx\n", inferior_elf.header.e_entry);
    printf("Program headers start: 0x%zx\n", inferior_elf.header.e_phoff);
    printf("Section headers start: 0x%zx\n", inferior_elf.header.e_shoff);
    printf("Section header size: %zd\n", inferior_elf.header.e_shentsize);
    printf("Section headers length: %zd\n", inferior_elf.header.e_shnum);

    /* Global Offset Table */

    printf("\nGOT Entries(%d):\n", inferior_elf.dynamic_data->got_entries_len);
    for (size_t i = 0; i < inferior_elf.dynamic_data->got_entries_len; i++) {
        struct GotEntry *got_entry = &inferior_elf.dynamic_data->got_entries[i];
        printf("%d: 0x%zx, 0x%zx\n", i, got_entry->index, got_entry->value);
    }
}
