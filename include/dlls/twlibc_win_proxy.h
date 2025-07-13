#pragma once

#include <types_linux.h>

ssize_t write(int32_t fd, const char *data, size_t length);

int32_t open(const char *path, int32_t flags);

int32_t close(int32_t fd);
