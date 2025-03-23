#include <stddef.h>

// @todo: use globally if we refactor "tiny_c_fprintf" to "fprintf"

#pragma once

#define TRACE 1
#define DEBUG 2
#define INFO 3
#define WARNING 4
#define ERROR 5
#define CRITICAL 6

#ifndef LOG_LEVEL
#define LOG_LEVEL WARNING
#endif

#if LOG_LEVEL <= TRACE

#define LOGTRACE(fmt, ...)                                                     \
    tiny_c_fprintf(2, "TRACE: ");                                              \
    tiny_c_fprintf(2, fmt, ##__VA_ARGS__);

#else

#define LOGTRACE(fmt, ...)                                                     \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= DEBUG

#define LOGDEBUG(fmt, ...)                                                     \
    tiny_c_fprintf(2, "DEBUG: ");                                              \
    tiny_c_fprintf(2, fmt, ##__VA_ARGS__);

#else

#define LOGDEBUG(fmt, ...)                                                     \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= INFO

#define LOGINFO(fmt, ...)                                                      \
    tiny_c_fprintf(2, "INFO: ");                                               \
    tiny_c_fprintf(2, fmt, ##__VA_ARGS__);

#else

#define LOGINFO(fmt, ...)                                                      \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= WARNING

#define LOGWARNING(fmt, ...)                                                   \
    tiny_c_fprintf(2, "WARNING: ");                                            \
    tiny_c_fprintf(2, fmt, ##__VA_ARGS__);

#else

#define LOGWARNING(fmt, ...)                                                   \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= ERROR

#define LOGERROR(fmt, ...)                                                     \
    tiny_c_fprintf(2, "ERROR: ");                                              \
    tiny_c_fprintf(2, fmt, ##__VA_ARGS__);

#else

#define LOGERROR(fmt, ...)                                                     \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif
