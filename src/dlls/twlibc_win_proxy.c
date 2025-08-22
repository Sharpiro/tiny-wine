#include <dlls/ntdll.h>
#include <dlls/twlibc_win_proxy.h>

// @todo: rm this proxy somehow?

// int32_t open(const char *path, int32_t flags) {
//     return open(path, flags);
// }

// size_t read(int32_t fd, void *buf, size_t count) {
//     return read(fd, buf, count);
// }
// off_t lseek(int32_t fd, off_t offset, int whence) {
//     return lseek(fd, offset, whence);
// }

// int32_t close(int32_t fd) {
//     return close(fd);
// }

// ssize_t write(int32_t fd, const char *data, size_t length) {
//     return write(fd, data, length);
// }
