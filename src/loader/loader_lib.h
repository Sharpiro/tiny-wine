#pragma once

#include <stdint.h>
#include <stdlib.h>

extern int32_t loader_log_handle;

void *loader_malloc_arena(size_t n);

void loader_free_arena(void);

// #define LOADER_LOG(fmt, ...)

#ifdef VERBOSE

#define LOADER_LOG(fmt, ...)                                                   \
    tiny_c_fprintf(loader_log_handle, fmt, ##__VA_ARGS__);

#else

// #define LOADER_LOG(fmt, ...)                                                   \
//     do {                                                                       \
//         (void)(fmt);                                                           \
//         if (0) {                                                               \
//             (void)0, ##__VA_ARGS__;                                            \
//         }                                                                      \
//     } while (0)

#define LOADER_LOG(fmt, ...)                                                   \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif
