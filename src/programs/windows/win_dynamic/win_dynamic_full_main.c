// #include "../../../dlls/msvcrt.h"
// #include "./win_dynamic_lib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct Xyz {
    int x;
    int *y;
};

int main() {
    // struct Xyz xyz = {};
    // int x = offsetof(struct Xyz, y);
    // const int OFFSET = offsetof(FILE, _fileno);
    // FILE *x = stdout;
    // printf("look how far we've come\n");

    fputc('Z', stdin);
    fputc('Z', stdin);
    fputc('Z', stdout);
    fputc('Z', stdout);
    fputc('Z', stderr);
    fputc('Z', stderr);
    // fileno()
    // _fileno("")
    // fprintf(stderr, "look how far we've come\n");

    // uint32_t *buffer = malloc(0x1000);
    // buffer[0] = 0xffffffff;
    // printf("malloc: %x\n", buffer[0]);
}

typedef struct {
    char *_ptr;      // Pointer to the current position in the buffer
    int _cnt;        // Number of bytes remaining in the buffer
    char *_base;     // Pointer to the buffer
    int _flag;       // Flags for the file status
    int _file;       // File descriptor
    int _charbuf;    // Single character buffer for ungetc
    int _bufsiz;     // Buffer size
    char *_tmpfname; // Pointer to the name of a temporary file
} temp;
