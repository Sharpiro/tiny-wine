#include <dlls/twlibc_linux.h>
#include <stddef.h>
#include <stdint.h>
#include <sys_linux.h>

extern int32_t errno_internal;

size_t brk(size_t brk) {
    return sys_brk(brk);
}

ssize_t write(int32_t fd, const char *data, size_t length) {
    sys_write(fd, data, length);
    return (ssize_t)length;
}

void *mmap(
    void *address,
    size_t length,
    int32_t prot,
    int32_t flags,
    int32_t fd,
    off_t offset
) {
    ssize_t result =
        (ssize_t)sys_mmap(address, length, prot, flags, fd, offset);
    if (result < 0) {
        errno_internal = -(int32_t)result;
        return MAP_FAILED;
    }

    return (void *)result;
}

int32_t open(const char *path, int32_t flags) {
    int32_t result = (int32_t)sys_open(path, flags);
    if (result < 0) {
        errno_internal = -result;
        return -1;
    }

    return result;
}

int32_t close(int32_t fd) {
    return (int32_t)sys_close(fd);
}

size_t read(int32_t fd, void *buf, size_t count) {
    return sys_read(fd, buf, count);
}

int32_t getpid(void) {
    return (int32_t)sys_getpid();
}

char *getcwd(char *buffer, size_t size) {
    size_t result = sys_getcwd(buffer, size);
    if ((int32_t)result < 0) {
        errno_internal = -(int32_t)result;
        return NULL;
    }

    return buffer;
}

int32_t uname(struct utsname *name) {
    int32_t result = (int32_t)sys_uname(name);
    if (result < 0) {
        errno_internal = -result;
        return -1;
    }

    return result;
}

uid_t getuid() {
    uid_t result = (uid_t)sys_getuid();
    return result;
}

off_t lseek(int32_t fd, off_t offset, int32_t whence) {
    off_t result = (off_t)sys_lseek(fd, offset, whence);
    if (result < 0) {
        errno_internal = -(int32_t)result;
        return -1;
    }
    return result;
}

int32_t munmap(void *address, size_t length) {
    int32_t result = (int32_t)sys_munmap(address, length);
    if (result < 0) {
        errno_internal = -result;
        return -1;
    }
    return 0;
}

int32_t mprotect(void *address, size_t length, int32_t protection) {
    int32_t result = (int32_t)sys_mprotect(address, length, protection);
    if (result < 0) {
        errno_internal = -result;
        return -1;
    }
    return 0;
}

int32_t arch_prctl(size_t code, size_t address) {
    int32_t result = (int32_t)sys_arch_prctl(code, address);
    if (result < 0) {
        errno_internal = -result;
        return -1;
    }
    return result;
}

void exit(int32_t code) {
    sys_exit(code);
}

void abort() {
    sys_exit(3);
}
