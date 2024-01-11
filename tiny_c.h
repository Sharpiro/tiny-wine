#include <stdint.h>
#include <stdlib.h>

void tiny_c_print_number(uint64_t num);
void tiny_c_newline();
void tiny_c_printf(const char *format, ...);
void tiny_c_exit(int code);
void *tiny_c_mmap(size_t address, size_t length, size_t prot, size_t flags,
                  uint64_t fd, size_t offset);
size_t tiny_c_munmap(size_t address, size_t length);
uint64_t tiny_c_fopen(const char *path);
void tiny_c_fclose(uint64_t fd);
