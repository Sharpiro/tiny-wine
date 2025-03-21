#pragma once

#include <stddef.h>

/**
 * By default variable exports will require relocations.
 * You can explicitly set export/import to avoid this.
 * If you use exportable on one function in a file, you must use it on all.
 * 'export' marks the symbol as exported in the dll.
 * 'import' tells the executable to reference it in its IAT.
 */
#ifdef _WIN32
#ifdef DLL
#define EXPORTABLE __declspec(dllexport)
#else
#define EXPORTABLE __declspec(dllimport)
#endif
#else
#define EXPORTABLE
#endif

#define GET_PRESERVED_REGISTERS()                                              \
    __asm__("mov %[rbx], rbx\n"                                                \
            "mov %[rcx], rcx\n"                                                \
            "mov %[rdx], rdx\n"                                                \
            "mov %[rdi], rdi\n"                                                \
            "mov %[rsi], rsi\n"                                                \
            "mov %[r8], r8\n"                                                  \
            "mov %[r9], r9\n"                                                  \
            "mov %[r12], r12\n"                                                \
            "mov %[r13], r13\n"                                                \
            "mov %[r14], r14\n"                                                \
            "mov %[r15], r15\n"                                                \
            "mov %[rbp], rbp\n"                                                \
            : [rbx] "=m"(rbx),                                                 \
              [rcx] "=m"(rcx),                                                 \
              [rdx] "=m"(rdx),                                                 \
              [rdi] "=m"(rdi),                                                 \
              [rsi] "=m"(rsi),                                                 \
              [r8] "=m"(r8),                                                   \
              [r9] "=m"(r9),                                                   \
              [r12] "=m"(r12),                                                 \
              [r13] "=m"(r13),                                                 \
              [r14] "=m"(r14),                                                 \
              [r15] "=m"(r15),                                                 \
              [rbp] "=m"(rbp))

#define DEBUG_SET_MACHINE_STATE()                                              \
    __asm__("mov rbx, 0x01\n"                                                  \
            "mov rcx, 0x00\n"                                                  \
            "mov rdx, 0x00\n"                                                  \
            "mov rdi, 0x02\n"                                                  \
            "mov rsi, 0x03\n"                                                  \
            "mov r8,  0x00\n"                                                  \
            "mov r9,  0x00\n"                                                  \
            "mov r12, 0x04\n"                                                  \
            "mov r13, 0x05\n"                                                  \
            "mov r14, 0x06\n"                                                  \
            "mov r15, 0x07\n"                                                  \
            "mov rbp, rbp\n"                                                   \
            :)
