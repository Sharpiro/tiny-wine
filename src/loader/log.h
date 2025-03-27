#include <stddef.h>

// @todo: use globally if we refactor "fprintf" to "fprintf"

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
    fprintf(stderr, "TRACE: ");                                                \
    fprintf(stderr, fmt, ##__VA_ARGS__);

#else

#define LOGTRACE(fmt, ...)                                                     \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= DEBUG

#define LOGDEBUG(fmt, ...)                                                     \
    fprintf(stderr, "DEBUG: ");                                                \
    fprintf(stderr, fmt, ##__VA_ARGS__);

#else

#define LOGDEBUG(fmt, ...)                                                     \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= INFO

#define LOGINFO(fmt, ...)                                                      \
    fprintf(stderr, "INFO: ");                                                 \
    fprintf(stderr, fmt, ##__VA_ARGS__);

#else

#define LOGINFO(fmt, ...)                                                      \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= WARNING

#define LOGWARNING(fmt, ...)                                                   \
    fprintf(stderr, "WARNING: ");                                              \
    fprintf(stderr, fmt, ##__VA_ARGS__);

#else

#define LOGWARNING(fmt, ...)                                                   \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif

#if LOG_LEVEL <= ERROR

#define LOGERROR(fmt, ...)                                                     \
    fprintf(stderr, "ERROR: ");                                                \
    fprintf(stderr, fmt, ##__VA_ARGS__);

#else

#define LOGERROR(fmt, ...)                                                     \
    if (0) {                                                                   \
        (void)0, ##__VA_ARGS__;                                                \
    }

#endif
