#pragma once

#include <stddef.h>
#include <string.h>

/* @note: old list never freed */

#define CREATE_LIST_STRUCT(type)                                               \
    typedef struct {                                                           \
        type *data;                                                            \
        size_t length;                                                         \
        size_t capacity;                                                       \
        void *(*allocator)(size_t n);                                          \
    } type##List;                                                              \
                                                                               \
    [[maybe_unused]] static inline bool type##List_add(                        \
        type##List *list, type data                                            \
    ) {                                                                        \
        const int MIN_CAPACITY = 16;                                           \
        if (list->length == list->capacity) {                                  \
            type *old_list_data = list->data;                                  \
            size_t old_list_size = sizeof(type) * list->capacity;              \
            list->capacity = list->capacity < MIN_CAPACITY                     \
                ? MIN_CAPACITY                                                 \
                : list->capacity * 2;                                          \
            list->data = list->allocator(sizeof(type) * list->capacity);       \
            if (list->data == NULL) {                                          \
                return false;                                                  \
            }                                                                  \
                                                                               \
            memcpy(list->data, old_list_data, old_list_size);                  \
        }                                                                      \
        list->data[list->length++] = data;                                     \
        return true;                                                           \
    }                                                                          \
                                                                               \
    [[maybe_unused]] static inline bool type##List_clone(                      \
        type##List *dest, type##List *src                                      \
    ) {                                                                        \
        for (size_t i = 0; i < src->length; i++) {                             \
            type *item = &src->data[i];                                        \
            type##List_add(dest, *item);                                       \
        }                                                                      \
        return false;                                                          \
    }
